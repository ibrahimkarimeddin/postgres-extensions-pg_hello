# Project Integration Guide

Complete guide for integrating the `pg_hello` extension into real-world applications and projects.

## 📋 Table of Contents

- [Integration Overview](#integration-overview)
- [Application Integration](#application-integration)
- [Framework Integration](#framework-integration)
- [Language-Specific Integration](#language-specific-integration)
- [ORM Integration](#orm-integration)
- [API Integration](#api-integration)
- [Microservices Integration](#microservices-integration)
- [Performance Considerations](#performance-considerations)
- [Security and Best Practices](#security-and-best-practices)

## 🔍 Integration Overview

### Integration Patterns

The `pg_hello` extension can be integrated into applications using several patterns:

1. **Direct SQL Usage** - Raw SQL queries in application code
2. **ORM Integration** - Through Object-Relational Mapping frameworks
3. **Stored Procedure Wrapper** - PostgreSQL functions that use the extension
4. **API Layer** - REST/GraphQL APIs that expose extension functionality
5. **Event-Driven** - Triggers and notifications using extension functions

### Use Cases

- **User Onboarding**: Personalized welcome messages
- **Logging and Audit**: Timestamped entries with millisecond precision
- **System Information**: Database version reporting
- **Configuration Management**: Dynamic greeting customization
- **Performance Testing**: Benchmarking with repeatable operations

## 🚀 Application Integration

### Web Application Example

```sql
-- Create application tables that use the extension
CREATE TABLE users (
    id SERIAL PRIMARY KEY,
    username VARCHAR(50) UNIQUE NOT NULL,
    email VARCHAR(100) NOT NULL,
    created_at BIGINT DEFAULT now_ms(),
    welcome_message TEXT
);

-- Trigger to automatically generate welcome messages
CREATE OR REPLACE FUNCTION generate_welcome_message()
RETURNS TRIGGER AS $$
BEGIN
    NEW.welcome_message := pg_hello(NEW.username);
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;

CREATE TRIGGER user_welcome_trigger
    BEFORE INSERT ON users
    FOR EACH ROW EXECUTE FUNCTION generate_welcome_message();

-- Test the integration
INSERT INTO users (username, email) VALUES 
    ('alice', 'alice@example.com'),
    ('bob', 'bob@example.com');

SELECT username, welcome_message, created_at FROM users;
```

### Configuration Management

```sql
-- Create configuration table
CREATE TABLE app_config (
    key VARCHAR(50) PRIMARY KEY,
    value TEXT NOT NULL,
    updated_at BIGINT DEFAULT now_ms()
);

-- Function to update greeting configuration
CREATE OR REPLACE FUNCTION update_greeting_config(repeat_count INTEGER)
RETURNS TEXT AS $$
DECLARE
    old_value TEXT;
    test_result TEXT;
BEGIN
    -- Store old value
    SELECT value INTO old_value FROM app_config WHERE key = 'greeting_repeat';
    
    -- Update configuration
    EXECUTE format('SET pg_hello.repeat = %s', repeat_count);
    
    -- Test new configuration
    SELECT pg_hello('Config Test') INTO test_result;
    
    -- Store in config table
    INSERT INTO app_config (key, value) 
    VALUES ('greeting_repeat', repeat_count::text)
    ON CONFLICT (key) DO UPDATE SET 
        value = EXCLUDED.value,
        updated_at = now_ms();
    
    RETURN format('Updated from %s to %s. Test: %s', 
                  COALESCE(old_value, 'default'), repeat_count, test_result);
END;
$$ LANGUAGE plpgsql;

-- Usage example
SELECT update_greeting_config(3);
```

## 🏗️ Framework Integration

### Node.js/Express Integration

```javascript
// package.json dependencies
{
  "dependencies": {
    "express": "^4.18.0",
    "pg": "^8.8.0",
    "dotenv": "^16.0.0"
  }
}

// app.js - Express application with pg_hello integration
const express = require('express');
const { Pool } = require('pg');
require('dotenv').config();

const app = express();
const pool = new Pool({
  user: process.env.DB_USER,
  host: process.env.DB_HOST,
  database: process.env.DB_NAME,
  password: process.env.DB_PASSWORD,
  port: process.env.DB_PORT,
});

app.use(express.json());

// Middleware to ensure extension is available
app.use(async (req, res, next) => {
  try {
    await pool.query('SELECT 1 FROM pg_extension WHERE extname = $1', ['pg_hello']);
    next();
  } catch (error) {
    res.status(500).json({ error: 'pg_hello extension not available' });
  }
});

// API endpoints using pg_hello extension
app.get('/api/greeting/:name', async (req, res) => {
  try {
    const { name } = req.params;
    const result = await pool.query('SELECT pg_hello($1) as greeting', [name]);
    res.json({ greeting: result.rows[0].greeting });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

app.post('/api/config/greeting', async (req, res) => {
  try {
    const { repeat } = req.body;
    await pool.query('SET pg_hello.repeat = $1', [repeat]);
    const test = await pool.query('SELECT pg_hello($1) as test', ['Config']);
    res.json({ 
      message: 'Configuration updated',
      test_result: test.rows[0].test 
    });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

app.get('/api/timestamp', async (req, res) => {
  try {
    const result = await pool.query('SELECT now_ms() as timestamp_ms');
    res.json({ timestamp_ms: result.rows[0].timestamp_ms });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

app.get('/api/info', async (req, res) => {
  try {
    const version = await pool.query('SELECT spi_version() as version');
    const extension = await pool.query(
      'SELECT extversion FROM pg_extension WHERE extname = $1', 
      ['pg_hello']
    );
    res.json({
      postgresql_version: version.rows[0].version,
      extension_version: extension.rows[0].extversion
    });
  } catch (error) {
    res.status(500).json({ error: error.message });
  }
});

const PORT = process.env.PORT || 3000;
app.listen(PORT, () => {
  console.log(`Server running on port ${PORT}`);
});
```

### Python/Flask Integration

```python
# requirements.txt
"""
Flask==2.3.0
psycopg2-binary==2.9.0
python-dotenv==1.0.0
"""

# app.py - Flask application with pg_hello integration
from flask import Flask, request, jsonify
import psycopg2
from psycopg2.extras import RealDictCursor
import os
from dotenv import load_dotenv

load_dotenv()

app = Flask(__name__)

# Database connection
def get_db_connection():
    return psycopg2.connect(
        host=os.getenv('DB_HOST'),
        database=os.getenv('DB_NAME'),
        user=os.getenv('DB_USER'),
        password=os.getenv('DB_PASSWORD'),
        port=os.getenv('DB_PORT')
    )

# Helper function to execute extension functions
def execute_pg_hello_function(query, params=None):
    try:
        conn = get_db_connection()
        cursor = conn.cursor(cursor_factory=RealDictCursor)
        cursor.execute(query, params)
        result = cursor.fetchone()
        conn.close()
        return result
    except Exception as e:
        raise Exception(f"Database error: {str(e)}")

@app.route('/api/greeting/<name>')
def get_greeting(name):
    try:
        result = execute_pg_hello_function(
            "SELECT pg_hello(%s) as greeting", 
            (name,)
        )
        return jsonify({'greeting': result['greeting']})
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/api/config/greeting', methods=['POST'])
def update_greeting_config():
    try:
        repeat = request.json.get('repeat', 1)
        
        conn = get_db_connection()
        cursor = conn.cursor(cursor_factory=RealDictCursor)
        
        # Update configuration
        cursor.execute("SET pg_hello.repeat = %s", (repeat,))
        
        # Test new configuration
        cursor.execute("SELECT pg_hello(%s) as test", ('Config',))
        test_result = cursor.fetchone()
        
        conn.close()
        
        return jsonify({
            'message': 'Configuration updated',
            'test_result': test_result['test']
        })
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/api/timestamp')
def get_timestamp():
    try:
        result = execute_pg_hello_function("SELECT now_ms() as timestamp_ms")
        return jsonify({'timestamp_ms': result['timestamp_ms']})
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/api/bulk-greetings', methods=['POST'])
def bulk_greetings():
    try:
        names = request.json.get('names', [])
        
        conn = get_db_connection()
        cursor = conn.cursor(cursor_factory=RealDictCursor)
        
        greetings = []
        for name in names:
            cursor.execute("SELECT pg_hello(%s) as greeting", (name,))
            result = cursor.fetchone()
            greetings.append({
                'name': name,
                'greeting': result['greeting']
            })
        
        conn.close()
        
        return jsonify({'greetings': greetings})
    except Exception as e:
        return jsonify({'error': str(e)}), 500

if __name__ == '__main__':
    app.run(debug=True)
```

### Java/Spring Boot Integration

```java
// pom.xml dependencies
/*
<dependencies>
    <dependency>
        <groupId>org.springframework.boot</groupId>
        <artifactId>spring-boot-starter-web</artifactId>
    </dependency>
    <dependency>
        <groupId>org.springframework.boot</groupId>
        <artifactId>spring-boot-starter-data-jpa</artifactId>
    </dependency>
    <dependency>
        <groupId>org.postgresql</groupId>
        <artifactId>postgresql</artifactId>
    </dependency>
</dependencies>
*/

// GreetingService.java
@Service
public class GreetingService {
    
    @Autowired
    private JdbcTemplate jdbcTemplate;
    
    public String getGreeting(String name) {
        return jdbcTemplate.queryForObject(
            "SELECT pg_hello(?) as greeting",
            String.class,
            name
        );
    }
    
    public void updateConfiguration(int repeat) {
        jdbcTemplate.update("SET pg_hello.repeat = ?", repeat);
    }
    
    public String testConfiguration(String testName) {
        return jdbcTemplate.queryForObject(
            "SELECT pg_hello(?) as test",
            String.class,
            testName
        );
    }
    
    public Long getCurrentTimestamp() {
        return jdbcTemplate.queryForObject(
            "SELECT now_ms() as timestamp_ms",
            Long.class
        );
    }
    
    public List<Map<String, Object>> getBulkGreetings(List<String> names) {
        StringBuilder sql = new StringBuilder("SELECT unnest(?) as name, pg_hello(unnest(?)) as greeting");
        
        Array nameArray = jdbcTemplate.getDataSource()
            .getConnection()
            .createArrayOf("text", names.toArray());
            
        return jdbcTemplate.queryForList(sql.toString(), nameArray, nameArray);
    }
}

// GreetingController.java
@RestController
@RequestMapping("/api")
public class GreetingController {
    
    @Autowired
    private GreetingService greetingService;
    
    @GetMapping("/greeting/{name}")
    public ResponseEntity<Map<String, String>> getGreeting(@PathVariable String name) {
        try {
            String greeting = greetingService.getGreeting(name);
            return ResponseEntity.ok(Map.of("greeting", greeting));
        } catch (Exception e) {
            return ResponseEntity.status(500)
                .body(Map.of("error", e.getMessage()));
        }
    }
    
    @PostMapping("/config/greeting")
    public ResponseEntity<Map<String, Object>> updateConfig(@RequestBody Map<String, Integer> config) {
        try {
            Integer repeat = config.get("repeat");
            greetingService.updateConfiguration(repeat);
            String testResult = greetingService.testConfiguration("Config");
            
            return ResponseEntity.ok(Map.of(
                "message", "Configuration updated",
                "test_result", testResult
            ));
        } catch (Exception e) {
            return ResponseEntity.status(500)
                .body(Map.of("error", e.getMessage()));
        }
    }
    
    @GetMapping("/timestamp")
    public ResponseEntity<Map<String, Long>> getTimestamp() {
        try {
            Long timestamp = greetingService.getCurrentTimestamp();
            return ResponseEntity.ok(Map.of("timestamp_ms", timestamp));
        } catch (Exception e) {
            return ResponseEntity.status(500)
                .body(Map.of("error", e.getMessage()));
        }
    }
}
```

## 🗃️ ORM Integration

### Django Integration

```python
# models.py
from django.db import models, connection
from django.db.models.signals import post_save
from django.dispatch import receiver

class User(models.Model):
    username = models.CharField(max_length=50, unique=True)
    email = models.EmailField()
    welcome_message = models.TextField(blank=True)
    created_at = models.BigIntegerField(null=True, blank=True)
    
    def save(self, *args, **kwargs):
        # Use pg_hello extension to generate welcome message
        if not self.welcome_message:
            with connection.cursor() as cursor:
                cursor.execute("SELECT pg_hello(%s)", [self.username])
                self.welcome_message = cursor.fetchone()[0]
        
        # Use now_ms() for timestamp
        if not self.created_at:
            with connection.cursor() as cursor:
                cursor.execute("SELECT now_ms()")
                self.created_at = cursor.fetchone()[0]
        
        super().save(*args, **kwargs)

# Custom manager for pg_hello functions
class GreetingManager(models.Manager):
    def create_with_greeting(self, username, email, repeat=1):
        with connection.cursor() as cursor:
            # Set configuration
            cursor.execute("SET pg_hello.repeat = %s", [repeat])
            
            # Create user
            user = self.create(username=username, email=email)
            return user
    
    def get_system_info(self):
        with connection.cursor() as cursor:
            cursor.execute("SELECT spi_version()")
            version = cursor.fetchone()[0]
            
            cursor.execute("SELECT extversion FROM pg_extension WHERE extname = 'pg_hello'")
            ext_version = cursor.fetchone()[0]
            
            return {
                'postgresql_version': version,
                'extension_version': ext_version
            }

User.add_to_class('greeting_manager', GreetingManager())

# views.py
from django.http import JsonResponse
from django.views.decorators.csrf import csrf_exempt
from django.db import connection
import json

@csrf_exempt
def greeting_api(request, name):
    if request.method == 'GET':
        with connection.cursor() as cursor:
            cursor.execute("SELECT pg_hello(%s)", [name])
            greeting = cursor.fetchone()[0]
        return JsonResponse({'greeting': greeting})

@csrf_exempt
def config_api(request):
    if request.method == 'POST':
        data = json.loads(request.body)
        repeat = data.get('repeat', 1)
        
        with connection.cursor() as cursor:
            cursor.execute("SET pg_hello.repeat = %s", [repeat])
            cursor.execute("SELECT pg_hello(%s)", ['Config'])
            test_result = cursor.fetchone()[0]
        
        return JsonResponse({
            'message': 'Configuration updated',
            'test_result': test_result
        })
```

### SQLAlchemy Integration

```python
# database.py
from sqlalchemy import create_engine, text, Column, Integer, String, BigInteger
from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy.orm import sessionmaker
from sqlalchemy.event import listens_for

Base = declarative_base()

class User(Base):
    __tablename__ = 'users'
    
    id = Column(Integer, primary_key=True)
    username = Column(String(50), unique=True, nullable=False)
    email = Column(String(100), nullable=False)
    welcome_message = Column(String)
    created_at = Column(BigInteger)

# Event listener to automatically set welcome message and timestamp
@listens_for(User, 'before_insert')
def receive_before_insert(mapper, connection, target):
    # Generate welcome message using pg_hello
    result = connection.execute(
        text("SELECT pg_hello(:username)"),
        {"username": target.username}
    )
    target.welcome_message = result.scalar()
    
    # Set timestamp using now_ms
    result = connection.execute(text("SELECT now_ms()"))
    target.created_at = result.scalar()

# Service class for pg_hello operations
class GreetingService:
    def __init__(self, session):
        self.session = session
    
    def get_greeting(self, name, repeat=None):
        if repeat:
            self.session.execute(text("SET pg_hello.repeat = :repeat"), {"repeat": repeat})
        
        result = self.session.execute(
            text("SELECT pg_hello(:name)"),
            {"name": name}
        )
        return result.scalar()
    
    def get_timestamp(self):
        result = self.session.execute(text("SELECT now_ms()"))
        return result.scalar()
    
    def get_system_info(self):
        version_result = self.session.execute(text("SELECT spi_version()"))
        ext_result = self.session.execute(
            text("SELECT extversion FROM pg_extension WHERE extname = 'pg_hello'")
        )
        
        return {
            'postgresql_version': version_result.scalar(),
            'extension_version': ext_result.scalar()
        }
    
    def bulk_greetings(self, names):
        # Use array operations for bulk processing
        result = self.session.execute(
            text("""
                SELECT name, pg_hello(name) as greeting
                FROM unnest(:names) as name
            """),
            {"names": names}
        )
        return [{"name": row[0], "greeting": row[1]} for row in result]

# Usage example
engine = create_engine('postgresql://user:password@localhost/mydb')
SessionLocal = sessionmaker(bind=engine)

def create_user_with_greeting(username, email):
    session = SessionLocal()
    try:
        user = User(username=username, email=email)
        session.add(user)
        session.commit()
        return user
    finally:
        session.close()
```

## 🔌 API Integration

### GraphQL Integration

```javascript
// GraphQL schema with pg_hello integration
const { gql } = require('apollo-server-express');
const { Pool } = require('pg');

const pool = new Pool({
  // database configuration
});

const typeDefs = gql`
  type Query {
    greeting(name: String!, repeat: Int = 1): GreetingResponse
    timestamp: TimestampResponse
    systemInfo: SystemInfo
    bulkGreetings(names: [String!]!): [GreetingResponse!]!
  }
  
  type Mutation {
    updateGreetingConfig(repeat: Int!): ConfigResponse
  }
  
  type GreetingResponse {
    name: String!
    greeting: String!
    timestamp: String!
  }
  
  type TimestampResponse {
    timestamp_ms: String!
  }
  
  type SystemInfo {
    postgresql_version: String!
    extension_version: String!
  }
  
  type ConfigResponse {
    message: String!
    test_result: String!
  }
`;

const resolvers = {
  Query: {
    greeting: async (_, { name, repeat }) => {
      const client = await pool.connect();
      try {
        if (repeat !== 1) {
          await client.query('SET pg_hello.repeat = $1', [repeat]);
        }
        
        const greetingResult = await client.query('SELECT pg_hello($1) as greeting', [name]);
        const timestampResult = await client.query('SELECT now_ms() as timestamp_ms');
        
        return {
          name,
          greeting: greetingResult.rows[0].greeting,
          timestamp: timestampResult.rows[0].timestamp_ms
        };
      } finally {
        client.release();
      }
    },
    
    timestamp: async () => {
      const result = await pool.query('SELECT now_ms() as timestamp_ms');
      return { timestamp_ms: result.rows[0].timestamp_ms };
    },
    
    systemInfo: async () => {
      const versionResult = await pool.query('SELECT spi_version() as version');
      const extResult = await pool.query(
        'SELECT extversion FROM pg_extension WHERE extname = $1',
        ['pg_hello']
      );
      
      return {
        postgresql_version: versionResult.rows[0].version,
        extension_version: extResult.rows[0].extversion
      };
    },
    
    bulkGreetings: async (_, { names }) => {
      const client = await pool.connect();
      try {
        const greetings = [];
        for (const name of names) {
          const greetingResult = await client.query('SELECT pg_hello($1) as greeting', [name]);
          const timestampResult = await client.query('SELECT now_ms() as timestamp_ms');
          
          greetings.push({
            name,
            greeting: greetingResult.rows[0].greeting,
            timestamp: timestampResult.rows[0].timestamp_ms
          });
        }
        return greetings;
      } finally {
        client.release();
      }
    }
  },
  
  Mutation: {
    updateGreetingConfig: async (_, { repeat }) => {
      const client = await pool.connect();
      try {
        await client.query('SET pg_hello.repeat = $1', [repeat]);
        const testResult = await client.query('SELECT pg_hello($1) as test', ['Config']);
        
        return {
          message: 'Configuration updated successfully',
          test_result: testResult.rows[0].test
        };
      } finally {
        client.release();
      }
    }
  }
};

module.exports = { typeDefs, resolvers };
```

### REST API with OpenAPI/Swagger

```yaml
# openapi.yaml - API specification
openapi: 3.0.0
info:
  title: pg_hello Extension API
  version: 1.0.0
  description: REST API for PostgreSQL pg_hello extension

paths:
  /api/greeting/{name}:
    get:
      summary: Get personalized greeting
      parameters:
        - name: name
          in: path
          required: true
          schema:
            type: string
        - name: repeat
          in: query
          required: false
          schema:
            type: integer
            minimum: 1
            maximum: 10
            default: 1
      responses:
        '200':
          description: Successful greeting
          content:
            application/json:
              schema:
                type: object
                properties:
                  greeting:
                    type: string
                  timestamp_ms:
                    type: integer
                    format: int64
  
  /api/config/greeting:
    post:
      summary: Update greeting configuration
      requestBody:
        required: true
        content:
          application/json:
            schema:
              type: object
              properties:
                repeat:
                  type: integer
                  minimum: 1
                  maximum: 10
      responses:
        '200':
          description: Configuration updated
          content:
            application/json:
              schema:
                type: object
                properties:
                  message:
                    type: string
                  test_result:
                    type: string
  
  /api/bulk-greetings:
    post:
      summary: Get multiple greetings
      requestBody:
        required: true
        content:
          application/json:
            schema:
              type: object
              properties:
                names:
                  type: array
                  items:
                    type: string
                repeat:
                  type: integer
                  default: 1
      responses:
        '200':
          description: Bulk greetings generated
          content:
            application/json:
              schema:
                type: object
                properties:
                  greetings:
                    type: array
                    items:
                      type: object
                      properties:
                        name:
                          type: string
                        greeting:
                          type: string
```

## 🏢 Microservices Integration

### Service-Oriented Architecture

```javascript
// greeting-service.js - Dedicated microservice for pg_hello
const express = require('express');
const { Pool } = require('pg');
const Redis = require('redis');

class GreetingService {
  constructor() {
    this.app = express();
    this.db = new Pool({
      connectionString: process.env.DATABASE_URL
    });
    this.cache = Redis.createClient({
      url: process.env.REDIS_URL
    });
    
    this.setupMiddleware();
    this.setupRoutes();
  }
  
  setupMiddleware() {
    this.app.use(express.json());
    this.app.use(this.extensionHealthCheck.bind(this));
  }
  
  async extensionHealthCheck(req, res, next) {
    try {
      const result = await this.db.query(
        'SELECT 1 FROM pg_extension WHERE extname = $1',
        ['pg_hello']
      );
      if (result.rows.length === 0) {
        return res.status(503).json({ error: 'pg_hello extension not available' });
      }
      next();
    } catch (error) {
      res.status(503).json({ error: 'Database connection failed' });
    }
  }
  
  setupRoutes() {
    // Health check endpoint
    this.app.get('/health', async (req, res) => {
      try {
        await this.db.query('SELECT pg_hello($1)', ['health']);
        res.json({ status: 'healthy', service: 'greeting-service' });
      } catch (error) {
        res.status(503).json({ status: 'unhealthy', error: error.message });
      }
    });
    
    // Greeting endpoint with caching
    this.app.get('/greeting/:name', async (req, res) => {
      const { name } = req.params;
      const repeat = parseInt(req.query.repeat) || 1;
      const cacheKey = `greeting:${name}:${repeat}`;
      
      try {
        // Check cache first
        const cached = await this.cache.get(cacheKey);
        if (cached) {
          return res.json({ 
            greeting: cached, 
            cached: true,
            timestamp_ms: Date.now()
          });
        }
        
        // Generate greeting
        const client = await this.db.connect();
        await client.query('SET pg_hello.repeat = $1', [repeat]);
        const result = await client.query('SELECT pg_hello($1) as greeting', [name]);
        const timestamp = await client.query('SELECT now_ms() as timestamp_ms');
        client.release();
        
        const greeting = result.rows[0].greeting;
        
        // Cache for 5 minutes
        await this.cache.setex(cacheKey, 300, greeting);
        
        res.json({
          greeting,
          cached: false,
          timestamp_ms: timestamp.rows[0].timestamp_ms
        });
      } catch (error) {
        res.status(500).json({ error: error.message });
      }
    });
    
    // Batch processing endpoint
    this.app.post('/greetings/batch', async (req, res) => {
      const { names, repeat = 1 } = req.body;
      
      try {
        const client = await this.db.connect();
        await client.query('SET pg_hello.repeat = $1', [repeat]);
        
        const greetings = await Promise.all(
          names.map(async (name) => {
            const result = await client.query('SELECT pg_hello($1) as greeting', [name]);
            return { name, greeting: result.rows[0].greeting };
          })
        );
        
        client.release();
        
        res.json({ greetings, count: greetings.length });
      } catch (error) {
        res.status(500).json({ error: error.message });
      }
    });
  }
  
  start(port = 3000) {
    this.app.listen(port, () => {
      console.log(`Greeting service running on port ${port}`);
    });
  }
}

// Start the service
if (require.main === module) {
  const service = new GreetingService();
  service.start(process.env.PORT || 3000);
}

module.exports = GreetingService;
```

### Docker Compose for Microservices

```yaml
# docker-compose.microservices.yml
version: '3.8'

services:
  postgres:
    image: postgres:15
    environment:
      POSTGRES_DB: microservices_db
      POSTGRES_USER: postgres
      POSTGRES_PASSWORD: password
    volumes:
      - ./init-pg-hello.sql:/docker-entrypoint-initdb.d/01-pg-hello.sql
      - postgres_data:/var/lib/postgresql/data
    healthcheck:
      test: ["CMD-SHELL", "pg_isready -U postgres -d microservices_db"]
      interval: 30s
      timeout: 10s
      retries: 3

  redis:
    image: redis:alpine
    healthcheck:
      test: ["CMD", "redis-cli", "ping"]
      interval: 30s
      timeout: 10s
      retries: 3

  greeting-service:
    build: ./services/greeting
    environment:
      DATABASE_URL: postgresql://postgres:password@postgres:5432/microservices_db
      REDIS_URL: redis://redis:6379
    depends_on:
      postgres:
        condition: service_healthy
      redis:
        condition: service_healthy
    ports:
      - "3001:3000"

  user-service:
    build: ./services/user
    environment:
      DATABASE_URL: postgresql://postgres:password@postgres:5432/microservices_db
      GREETING_SERVICE_URL: http://greeting-service:3000
    depends_on:
      - postgres
      - greeting-service
    ports:
      - "3002:3000"

  api-gateway:
    build: ./services/gateway
    environment:
      GREETING_SERVICE_URL: http://greeting-service:3000
      USER_SERVICE_URL: http://user-service:3000
    depends_on:
      - greeting-service
      - user-service
    ports:
      - "3000:3000"

volumes:
  postgres_data:
```

## ⚡ Performance Considerations

### Connection Pooling

```javascript
// optimized-pool.js
const { Pool } = require('pg');

class OptimizedGreetingService {
  constructor() {
    // Connection pool configuration
    this.pool = new Pool({
      host: process.env.DB_HOST,
      database: process.env.DB_NAME,
      user: process.env.DB_USER,
      password: process.env.DB_PASSWORD,
      port: process.env.DB_PORT,
      
      // Pool configuration
      max: 20,                    // Maximum connections
      idleTimeoutMillis: 30000,   // Close idle connections after 30s
      connectionTimeoutMillis: 2000, // Fail after 2s if no connection available
      maxUses: 7500,              // Close connection after 7500 uses
      
      // Performance tuning
      application_name: 'greeting_service',
      statement_timeout: 5000,     // 5 second statement timeout
    });
    
    // Pre-configure session for performance
    this.pool.on('connect', async (client) => {
      // Set default configuration for all connections
      await client.query('SET pg_hello.repeat = 1');
      await client.query('SET statement_timeout = 5000');
    });
  }
  
  async getGreetingOptimized(name, repeat = 1) {
    const client = await this.pool.connect();
    try {
      // Batch operations for efficiency
      const queries = [
        repeat !== 1 ? `SET pg_hello.repeat = ${repeat}` : null,
        `SELECT pg_hello('${name}') as greeting, now_ms() as timestamp_ms`
      ].filter(Boolean);
      
      let result;
      for (const query of queries) {
        result = await client.query(query);
      }
      
      return {
        greeting: result.rows[0].greeting,
        timestamp_ms: result.rows[0].timestamp_ms
      };
    } finally {
      client.release();
    }
  }
  
  async getBulkGreetingsOptimized(names, repeat = 1) {
    const client = await this.pool.connect();
    try {
      // Use array operations for bulk processing
      if (repeat !== 1) {
        await client.query('SET pg_hello.repeat = $1', [repeat]);
      }
      
      const result = await client.query(`
        SELECT 
          name,
          pg_hello(name) as greeting,
          now_ms() as timestamp_ms
        FROM unnest($1::text[]) as name
      `, [names]);
      
      return result.rows;
    } finally {
      client.release();
    }
  }
}
```

### Caching Strategies

```javascript
// caching-service.js
const Redis = require('redis');

class CachedGreetingService {
  constructor(dbPool) {
    this.db = dbPool;
    this.cache = Redis.createClient({
      url: process.env.REDIS_URL,
      retry_delay_on_failure: 100,
      max_retry_delay: 1000,
    });
  }
  
  async getGreetingWithCache(name, repeat = 1, ttl = 300) {
    const cacheKey = `greeting:${name}:${repeat}`;
    
    try {
      // Try cache first
      const cached = await this.cache.get(cacheKey);
      if (cached) {
        return {
          ...JSON.parse(cached),
          cached: true
        };
      }
    } catch (cacheError) {
      console.warn('Cache error:', cacheError.message);
    }
    
    // Generate greeting from database
    const client = await this.db.connect();
    try {
      if (repeat !== 1) {
        await client.query('SET pg_hello.repeat = $1', [repeat]);
      }
      
      const result = await client.query(`
        SELECT 
          pg_hello($1) as greeting,
          now_ms() as timestamp_ms
      `, [name]);
      
      const greeting = {
        greeting: result.rows[0].greeting,
        timestamp_ms: result.rows[0].timestamp_ms,
        cached: false
      };
      
      // Cache the result
      try {
        await this.cache.setex(cacheKey, ttl, JSON.stringify(greeting));
      } catch (cacheError) {
        console.warn('Cache write error:', cacheError.message);
      }
      
      return greeting;
    } finally {
      client.release();
    }
  }
  
  async invalidateGreetingCache(name, repeat = null) {
    try {
      if (repeat !== null) {
        await this.cache.del(`greeting:${name}:${repeat}`);
      } else {
        // Invalidate all variants for this name
        const pattern = `greeting:${name}:*`;
        const keys = await this.cache.keys(pattern);
        if (keys.length > 0) {
          await this.cache.del(...keys);
        }
      }
    } catch (error) {
      console.warn('Cache invalidation error:', error.message);
    }
  }
}
```

## 🔒 Security and Best Practices

### Input Validation and Sanitization

```javascript
// security.js
const validator = require('validator');

class SecureGreetingService {
  validateName(name) {
    // Input validation
    if (!name || typeof name !== 'string') {
      throw new Error('Name must be a non-empty string');
    }
    
    if (name.length > 50) {
      throw new Error('Name must not exceed 50 characters');
    }
    
    // Sanitize input
    const sanitized = validator.escape(name.trim());
    
    // Additional security checks
    if (sanitized.includes('--') || sanitized.includes('/*')) {
      throw new Error('Invalid characters in name');
    }
    
    return sanitized;
  }
  
  validateRepeat(repeat) {
    const num = parseInt(repeat);
    if (isNaN(num) || num < 1 || num > 10) {
      throw new Error('Repeat must be an integer between 1 and 10');
    }
    return num;
  }
  
  async getSecureGreeting(name, repeat = 1) {
    // Validate inputs
    const safeName = this.validateName(name);
    const safeRepeat = this.validateRepeat(repeat);
    
    const client = await this.db.connect();
    try {
      // Use parameterized queries
      if (safeRepeat !== 1) {
        await client.query('SET pg_hello.repeat = $1', [safeRepeat]);
      }
      
      const result = await client.query(
        'SELECT pg_hello($1) as greeting, now_ms() as timestamp_ms',
        [safeName]
      );
      
      return {
        greeting: result.rows[0].greeting,
        timestamp_ms: result.rows[0].timestamp_ms
      };
    } finally {
      client.release();
    }
  }
}
```

### Rate Limiting

```javascript
// rate-limiting.js
const rateLimit = require('express-rate-limit');
const slowDown = require('express-slow-down');

// Rate limiting middleware
const greetingRateLimit = rateLimit({
  windowMs: 15 * 60 * 1000, // 15 minutes
  max: 100, // Limit each IP to 100 requests per windowMs
  message: {
    error: 'Too many greeting requests from this IP, please try again later.'
  },
  standardHeaders: true,
  legacyHeaders: false,
});

// Speed limiting middleware
const greetingSpeedLimit = slowDown({
  windowMs: 15 * 60 * 1000, // 15 minutes
  delayAfter: 50, // Allow 50 requests per 15 minutes at full speed
  delayMs: 500, // Add 500ms delay after 50 requests
  maxDelayMs: 20000, // Maximum delay of 20 seconds
});

// Apply to greeting endpoints
app.use('/api/greeting', greetingRateLimit, greetingSpeedLimit);
```

This comprehensive integration guide shows how to use the `pg_hello` extension in real-world applications across different frameworks, languages, and architectures! 🚀

# Deployment and Production Guide

Complete guide for deploying the `pg_hello` extension in production environments, from development to enterprise deployment.

## 📋 Table of Contents

- [Deployment Overview](#deployment-overview)
- [Development Environment](#development-environment)
- [Testing Environment](#testing-environment)
- [Staging Environment](#staging-environment)
- [Production Deployment](#production-deployment)
- [Global Installation](#global-installation)
- [Container Deployment](#container-deployment)
- [Cloud Platform Deployment](#cloud-platform-deployment)
- [Monitoring and Maintenance](#monitoring-and-maintenance)
- [Backup and Recovery](#backup-and-recovery)

## 🔍 Deployment Overview

### Deployment Stages

```
Development → Testing → Staging → Production
     ↓           ↓        ↓          ↓
  Local Dev   Unit Tests  UAT    Live System
  Quick Test  Integration Load Test  Full Load
  Iteration   Validation  Sign-off   Monitoring
```

### Environment Requirements

| Environment | Purpose | Requirements | Extension Scope |
|-------------|---------|--------------|-----------------|
| **Development** | Feature development | Local PostgreSQL | Single database |
| **Testing** | Automated testing | CI/CD PostgreSQL | Test databases |
| **Staging** | User acceptance | Production-like setup | Multiple databases |
| **Production** | Live system | High availability | All databases |

## 🔧 Development Environment

### Local Development Setup

```bash
# 1. Set up development directory
mkdir -p ~/dev/postgresql-extensions
cd ~/dev/postgresql-extensions
git clone <your-extension-repo> pg-hello
cd pg-hello

# 2. Install PostgreSQL development environment
# macOS:
brew install postgresql@15

# Ubuntu/Debian:
sudo apt-get install postgresql-15 postgresql-server-dev-15

# CentOS/RHEL:
sudo yum install postgresql15-server postgresql15-devel

# 3. Build and install extension
make clean && make && sudo make install

# 4. Create development database
createdb myapp_dev
psql myapp_dev -c "CREATE EXTENSION pg_hello;"
```

### Development Workflow

```bash
# Daily development cycle
git pull origin main                    # Get latest changes
make clean && make                      # Build extension
sudo make install                       # Install globally
psql myapp_dev -c "DROP EXTENSION IF EXISTS pg_hello CASCADE; CREATE EXTENSION pg_hello;" # Reload in dev DB
psql myapp_dev -c "SELECT pg_hello('Development Test');"  # Test functionality
```

### Development Database Setup

```sql
-- Create dedicated development database
CREATE DATABASE myapp_development;

-- Connect to development database
\c myapp_development

-- Install extension
CREATE EXTENSION pg_hello;

-- Set development-friendly configuration
SET pg_hello.repeat = 2;

-- Test basic functionality
SELECT pg_hello('Dev Environment') AS greeting;
SELECT now_ms() AS current_time_ms;
SELECT spi_version() AS pg_version;
```

## 🧪 Testing Environment

### Automated Testing Setup

```bash
# Create test database
createdb pg_hello_test

# Run regression tests
make installcheck

# Custom test script
cat > test_runner.sh << 'EOF'
#!/bin/bash
set -e

echo "Starting extension tests..."

# Create test database
dropdb --if-exists pg_hello_test
createdb pg_hello_test

# Install extension
psql pg_hello_test -c "CREATE EXTENSION pg_hello;"

# Run tests
psql pg_hello_test -f test/sql/basic.sql

# Performance test
psql pg_hello_test -c "
  \timing on
  SELECT COUNT(*) FROM (
    SELECT pg_hello('Performance Test ' || i::text) 
    FROM generate_series(1, 1000) i
  ) t;
"

echo "All tests passed!"
EOF

chmod +x test_runner.sh
./test_runner.sh
```

### CI/CD Integration

#### GitHub Actions Example

```yaml
# .github/workflows/test.yml
name: PostgreSQL Extension CI

on: [push, pull_request]

jobs:
  test:
    runs-on: ubuntu-latest
    
    services:
      postgres:
        image: postgres:15
        env:
          POSTGRES_PASSWORD: postgres
          POSTGRES_DB: test
        options: >-
          --health-cmd pg_isready
          --health-interval 10s
          --health-timeout 5s
          --health-retries 5
        ports:
          - 5432:5432
    
    steps:
    - uses: actions/checkout@v3
    
    - name: Install PostgreSQL development files
      run: |
        sudo apt-get update
        sudo apt-get install postgresql-server-dev-15
    
    - name: Build extension
      run: |
        make clean
        make
    
    - name: Install extension
      run: sudo make install
    
    - name: Test extension
      run: |
        PGPASSWORD=postgres createdb -h localhost -U postgres pg_hello_test
        PGPASSWORD=postgres psql -h localhost -U postgres pg_hello_test -c "CREATE EXTENSION pg_hello;"
        PGPASSWORD=postgres make installcheck
      env:
        PGUSER: postgres
        PGHOST: localhost
        PGDATABASE: pg_hello_test
```

#### Jenkins Pipeline

```groovy
// Jenkinsfile
pipeline {
    agent any
    
    environment {
        PGUSER = 'postgres'
        PGHOST = 'localhost'
        PGDATABASE = 'pg_hello_test'
    }
    
    stages {
        stage('Setup') {
            steps {
                sh 'sudo apt-get update'
                sh 'sudo apt-get install -y postgresql-server-dev-15'
            }
        }
        
        stage('Build') {
            steps {
                sh 'make clean'
                sh 'make'
            }
        }
        
        stage('Install') {
            steps {
                sh 'sudo make install'
            }
        }
        
        stage('Test') {
            steps {
                sh 'createdb pg_hello_test || true'
                sh 'psql pg_hello_test -c "DROP EXTENSION IF EXISTS pg_hello; CREATE EXTENSION pg_hello;"'
                sh 'make installcheck'
            }
        }
    }
    
    post {
        always {
            sh 'dropdb pg_hello_test || true'
        }
    }
}
```

## 🎭 Staging Environment

### Staging Setup

```bash
# Staging environment should mirror production
# 1. Same PostgreSQL version
# 2. Same hardware specs (or scaled down proportionally)
# 3. Same network configuration
# 4. Production-like data volume

# Create staging database cluster
sudo -u postgres initdb -D /var/lib/postgresql/staging/data
sudo -u postgres pg_ctl -D /var/lib/postgresql/staging/data -l /var/log/postgresql/staging.log start

# Install extension in staging
make install PG_CONFIG=/usr/pgsql-15/bin/pg_config

# Create staging databases
createdb myapp_staging
psql myapp_staging -c "CREATE EXTENSION pg_hello;"
```

### Load Testing

```sql
-- Create load testing script
\timing on

-- Test with realistic data volume
CREATE TABLE test_greetings AS 
SELECT 
    i AS id,
    'User' || i AS username,
    pg_hello('User' || i) AS greeting,
    now_ms() AS created_at
FROM generate_series(1, 100000) i;

-- Performance analysis
EXPLAIN ANALYZE 
SELECT COUNT(*) FROM test_greetings 
WHERE greeting LIKE 'Hello, User%';

-- Configuration testing
SET pg_hello.repeat = 5;
SELECT pg_hello('Load Test') AS multi_greeting;

-- Cleanup
DROP TABLE test_greetings;
```

### User Acceptance Testing

```bash
# UAT script for business users
cat > uat_tests.sql << 'EOF'
-- User Acceptance Test Suite
\echo 'Starting User Acceptance Tests...'

-- Test 1: Basic greeting functionality
\echo 'Test 1: Basic greeting'
SELECT pg_hello('Alice') AS greeting_test;

-- Test 2: Configuration changes
\echo 'Test 2: Configuration'
SET pg_hello.repeat = 3;
SELECT pg_hello('Configuration') AS config_test;

-- Test 3: Integration with existing data
\echo 'Test 3: Integration test'
SELECT 
    username,
    pg_hello(username) AS personalized_greeting
FROM (VALUES ('Admin'), ('Manager'), ('User')) AS t(username);

-- Test 4: Performance with realistic data
\echo 'Test 4: Performance test'
SELECT COUNT(*) FROM (
    SELECT pg_hello('Performance' || i::text)
    FROM generate_series(1, 1000) i
) t;

\echo 'User Acceptance Tests Complete!'
EOF

psql myapp_staging -f uat_tests.sql
```

## 🚀 Production Deployment

### Pre-Production Checklist

```bash
# Deployment checklist script
cat > pre_deployment_check.sh << 'EOF'
#!/bin/bash

echo "=== Pre-Production Deployment Checklist ==="

# 1. PostgreSQL version compatibility
echo "1. Checking PostgreSQL version..."
PGVERSION=$(psql --version | awk '{print $3}' | cut -d. -f1,2)
echo "PostgreSQL version: $PGVERSION"

# 2. Extension files exist
echo "2. Checking extension files..."
SHAREDIR=$(pg_config --sharedir)
LIBDIR=$(pg_config --libdir)

if [ -f "$SHAREDIR/extension/pg_hello.control" ]; then
    echo "✓ Control file exists"
else
    echo "✗ Control file missing"
    exit 1
fi

if [ -f "$SHAREDIR/extension/pg_hello--1.0.sql" ]; then
    echo "✓ SQL file exists"
else
    echo "✗ SQL file missing"
    exit 1
fi

if [ -f "$LIBDIR/pg_hello.so" ]; then
    echo "✓ Shared library exists"
else
    echo "✗ Shared library missing"
    exit 1
fi

# 3. Test extension creation
echo "3. Testing extension creation..."
psql template1 -c "CREATE EXTENSION IF NOT EXISTS pg_hello;" 2>/dev/null
if [ $? -eq 0 ]; then
    echo "✓ Extension can be created"
    psql template1 -c "DROP EXTENSION pg_hello;" 2>/dev/null
else
    echo "✗ Extension creation failed"
    exit 1
fi

# 4. Performance baseline
echo "4. Performance baseline test..."
psql template1 -c "CREATE EXTENSION pg_hello; SELECT pg_hello('Performance') FROM generate_series(1,100); DROP EXTENSION pg_hello;" > /dev/null

echo "✓ All pre-deployment checks passed!"
EOF

chmod +x pre_deployment_check.sh
./pre_deployment_check.sh
```

### Production Installation

```bash
# Production deployment script
cat > deploy_production.sh << 'EOF'
#!/bin/bash
set -e

echo "=== Production Deployment Script ==="

# 1. Create backup of current state
echo "1. Creating backup..."
pg_dumpall --globals-only > /backup/globals_$(date +%Y%m%d_%H%M%S).sql

# 2. Build extension
echo "2. Building extension..."
make clean
make

# 3. Install extension system-wide
echo "3. Installing extension..."
sudo make install

# 4. Verify installation
echo "4. Verifying installation..."
psql template1 -c "CREATE EXTENSION IF NOT EXISTS pg_hello; DROP EXTENSION pg_hello;"

# 5. Install in production databases
echo "5. Installing in production databases..."
for DB in myapp_prod myapp_analytics myapp_reporting; do
    echo "Installing in $DB..."
    psql $DB -c "CREATE EXTENSION IF NOT EXISTS pg_hello;"
    psql $DB -c "SELECT extversion FROM pg_extension WHERE extname = 'pg_hello';"
done

echo "✓ Production deployment complete!"
EOF

chmod +x deploy_production.sh
```

### Rolling Deployment

```bash
# For high-availability environments
cat > rolling_deploy.sh << 'EOF'
#!/bin/bash

# Rolling deployment for PostgreSQL cluster
SERVERS=("pg-primary" "pg-replica1" "pg-replica2")

for SERVER in "${SERVERS[@]}"; do
    echo "Deploying to $SERVER..."
    
    # Build and install on each server
    ssh $SERVER "cd /opt/pg_hello && git pull && make clean && make && sudo make install"
    
    # For replicas, they'll get the extension when they sync
    if [ "$SERVER" = "pg-primary" ]; then
        ssh $SERVER "psql myapp_prod -c 'CREATE EXTENSION IF NOT EXISTS pg_hello;'"
    fi
    
    # Verify deployment
    ssh $SERVER "psql template1 -c 'CREATE EXTENSION IF NOT EXISTS pg_hello; DROP EXTENSION pg_hello;'"
    
    echo "✓ $SERVER deployment complete"
done
EOF
```

## 🌍 Global Installation

### System-Wide Installation

```bash
# Install for all PostgreSQL instances on the system
sudo make install

# Verify global installation
ls -la $(pg_config --sharedir)/extension/pg_hello*
ls -la $(pg_config --libdir)/pg_hello*

# Make available to all databases
sudo -u postgres psql template1 -c "CREATE EXTENSION IF NOT EXISTS pg_hello;"
```

### Multi-Version PostgreSQL Support

```bash
# Install for multiple PostgreSQL versions
for PG_VERSION in 13 14 15; do
    echo "Installing for PostgreSQL $PG_VERSION..."
    make clean
    PG_CONFIG=/usr/pgsql-$PG_VERSION/bin/pg_config make install
done

# Verify each installation
for PG_VERSION in 13 14 15; do
    /usr/pgsql-$PG_VERSION/bin/psql template1 -c "CREATE EXTENSION IF NOT EXISTS pg_hello; DROP EXTENSION pg_hello;"
done
```

### Package-Based Installation

#### RPM Package (CentOS/RHEL)

```bash
# Create RPM spec file
cat > postgresql-pg_hello.spec << 'EOF'
Name:           postgresql-pg_hello
Version:        1.0
Release:        1%{?dist}
Summary:        PostgreSQL pg_hello extension

License:        PostgreSQL
URL:            https://github.com/yourorg/pg_hello
Source0:        pg_hello-%{version}.tar.gz

BuildRequires:  postgresql-devel
Requires:       postgresql-server

%description
A sample PostgreSQL extension providing greeting functions.

%prep
%setup -q -n pg_hello-%{version}

%build
make %{?_smp_mflags}

%install
make install DESTDIR=%{buildroot}

%files
%{_datadir}/postgresql/extension/pg_hello*
%{_libdir}/postgresql/pg_hello.so

%changelog
* Wed Oct 25 2023 Developer <dev@example.com> - 1.0-1
- Initial package
EOF

# Build RPM
rpmbuild -ba postgresql-pg_hello.spec

# Install RPM
sudo rpm -i ~/rpmbuild/RPMS/x86_64/postgresql-pg_hello-1.0-1.el8.x86_64.rpm
```

#### DEB Package (Ubuntu/Debian)

```bash
# Create debian package structure
mkdir -p pg-hello-1.0/debian
cd pg-hello-1.0

# Create control file
cat > debian/control << 'EOF'
Source: pg-hello
Section: database
Priority: optional
Maintainer: Your Name <your@email.com>
Build-Depends: debhelper (>= 9), postgresql-server-dev-all
Standards-Version: 3.9.6

Package: postgresql-pg-hello
Architecture: any
Depends: ${shlibs:Depends}, ${misc:Depends}, postgresql-common
Description: PostgreSQL pg_hello extension
 A sample PostgreSQL extension providing greeting functions.
EOF

# Build package
dpkg-buildpackage -b

# Install package
sudo dpkg -i ../postgresql-pg-hello_1.0-1_amd64.deb
```

## 🐳 Container Deployment

### Docker Image

```dockerfile
# Dockerfile for PostgreSQL with pg_hello extension
FROM postgres:15

# Install build dependencies
RUN apt-get update && apt-get install -y \
    postgresql-server-dev-15 \
    build-essential \
    && rm -rf /var/lib/apt/lists/*

# Copy extension source
COPY . /tmp/pg_hello

# Build and install extension
WORKDIR /tmp/pg_hello
RUN make clean && make && make install

# Create initialization script
RUN echo "CREATE EXTENSION IF NOT EXISTS pg_hello;" > /docker-entrypoint-initdb.d/01-pg_hello.sql

# Cleanup
RUN apt-get remove -y postgresql-server-dev-15 build-essential && \
    apt-get autoremove -y && \
    rm -rf /tmp/pg_hello

WORKDIR /
```

### Docker Compose

```yaml
# docker-compose.yml
version: '3.8'

services:
  postgres:
    build: .
    environment:
      POSTGRES_DB: myapp
      POSTGRES_USER: myapp
      POSTGRES_PASSWORD: secure_password
    ports:
      - "5432:5432"
    volumes:
      - postgres_data:/var/lib/postgresql/data
      - ./init-scripts:/docker-entrypoint-initdb.d
    healthcheck:
      test: ["CMD-SHELL", "pg_isready -U myapp -d myapp"]
      interval: 30s
      timeout: 10s
      retries: 3

  app:
    image: myapp:latest
    depends_on:
      postgres:
        condition: service_healthy
    environment:
      DATABASE_URL: postgresql://myapp:secure_password@postgres:5432/myapp

volumes:
  postgres_data:
```

### Kubernetes Deployment

```yaml
# k8s-deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: postgres-with-pg-hello
spec:
  replicas: 1
  selector:
    matchLabels:
      app: postgres
  template:
    metadata:
      labels:
        app: postgres
    spec:
      containers:
      - name: postgres
        image: your-registry/postgres-pg-hello:1.0
        env:
        - name: POSTGRES_DB
          value: "myapp"
        - name: POSTGRES_USER
          value: "myapp"
        - name: POSTGRES_PASSWORD
          valueFrom:
            secretKeyRef:
              name: postgres-secret
              key: password
        ports:
        - containerPort: 5432
        volumeMounts:
        - name: postgres-data
          mountPath: /var/lib/postgresql/data
      volumes:
      - name: postgres-data
        persistentVolumeClaim:
          claimName: postgres-pvc
---
apiVersion: v1
kind: Service
metadata:
  name: postgres-service
spec:
  selector:
    app: postgres
  ports:
  - port: 5432
    targetPort: 5432
  type: ClusterIP
```

## ☁️ Cloud Platform Deployment

### AWS RDS

```bash
# For AWS RDS, extensions must be whitelisted
# pg_hello would need to be added to AWS's extension library
# Alternative: Use AWS RDS Custom or EC2 with self-managed PostgreSQL

# Self-managed on EC2
aws ec2 run-instances \
    --image-id ami-0c02fb55956c7d316 \
    --instance-type t3.medium \
    --key-name my-key \
    --security-group-ids sg-12345678 \
    --user-data file://install-pg-hello.sh
```

### Google Cloud SQL

```bash
# Similar to AWS RDS, requires extension to be whitelisted
# Alternative: Use Compute Engine with PostgreSQL

gcloud compute instances create postgres-with-pg-hello \
    --machine-type n1-standard-2 \
    --image-family ubuntu-2004-lts \
    --image-project ubuntu-os-cloud \
    --metadata-from-file startup-script=install-pg-hello.sh
```

### Azure Database for PostgreSQL

```bash
# Use Flexible Server for extension support
az postgres flexible-server create \
    --resource-group myResourceGroup \
    --name myserver \
    --admin-user myadmin \
    --admin-password mypassword \
    --sku-name Standard_D2s_v3 \
    --version 15
```

## 📊 Monitoring and Maintenance

### Health Checks

```sql
-- Health check queries
-- 1. Extension is installed
SELECT extname, extversion 
FROM pg_extension 
WHERE extname = 'pg_hello';

-- 2. Functions are available
\df pg_hello

-- 3. Basic functionality works
SELECT pg_hello('Health Check') AS test_result;

-- 4. Configuration is correct
SHOW pg_hello.repeat;
```

### Performance Monitoring

```sql
-- Monitor extension function performance
SELECT 
    schemaname,
    funcname,
    calls,
    total_time,
    mean_time,
    stddev_time
FROM pg_stat_user_functions 
WHERE funcname LIKE 'pg_hello%';

-- Reset statistics
SELECT pg_stat_reset();
```

### Log Monitoring

```bash
# Monitor PostgreSQL logs for extension-related issues
tail -f /var/log/postgresql/postgresql.log | grep -i "pg_hello\|extension"

# Specific error patterns
grep -E "(ERROR|FATAL|PANIC).*pg_hello" /var/log/postgresql/postgresql.log
```

### Automated Monitoring Script

```bash
cat > monitor_pg_hello.sh << 'EOF'
#!/bin/bash

DATABASES=("myapp_prod" "myapp_analytics")
LOGFILE="/var/log/pg_hello_monitor.log"

for DB in "${DATABASES[@]}"; do
    echo "$(date): Checking $DB" >> $LOGFILE
    
    # Check extension exists
    RESULT=$(psql $DB -t -c "SELECT COUNT(*) FROM pg_extension WHERE extname = 'pg_hello';")
    if [ "$RESULT" -ne 1 ]; then
        echo "$(date): ERROR - pg_hello not installed in $DB" >> $LOGFILE
        # Send alert
        echo "pg_hello extension missing in $DB" | mail -s "Extension Alert" admin@company.com
    fi
    
    # Check function works
    psql $DB -c "SELECT pg_hello('Monitor Test');" > /dev/null 2>&1
    if [ $? -ne 0 ]; then
        echo "$(date): ERROR - pg_hello function failed in $DB" >> $LOGFILE
    else
        echo "$(date): OK - pg_hello working in $DB" >> $LOGFILE
    fi
done
EOF

# Add to crontab
echo "*/5 * * * * /usr/local/bin/monitor_pg_hello.sh" | crontab -
```

## 💾 Backup and Recovery

### Extension-Aware Backups

```bash
# Backup script that includes extension metadata
cat > backup_with_extensions.sh << 'EOF'
#!/bin/bash

BACKUP_DIR="/backup/$(date +%Y%m%d)"
mkdir -p $BACKUP_DIR

# 1. Backup database with extension data
pg_dump myapp_prod > $BACKUP_DIR/myapp_prod.sql

# 2. Backup extension files
cp $(pg_config --sharedir)/extension/pg_hello* $BACKUP_DIR/
cp $(pg_config --libdir)/pg_hello.so $BACKUP_DIR/

# 3. Backup configuration
psql myapp_prod -c "SHOW ALL;" | grep pg_hello > $BACKUP_DIR/pg_hello_config.txt

echo "Backup completed: $BACKUP_DIR"
EOF
```

### Recovery Procedures

```bash
# Recovery script
cat > restore_with_extensions.sh << 'EOF'
#!/bin/bash

BACKUP_DIR=$1
if [ -z "$BACKUP_DIR" ]; then
    echo "Usage: $0 /path/to/backup"
    exit 1
fi

# 1. Restore extension files
sudo cp $BACKUP_DIR/pg_hello.control $(pg_config --sharedir)/extension/
sudo cp $BACKUP_DIR/pg_hello--*.sql $(pg_config --sharedir)/extension/
sudo cp $BACKUP_DIR/pg_hello.so $(pg_config --libdir)/

# 2. Create database and restore data
createdb myapp_prod_restored
psql myapp_prod_restored < $BACKUP_DIR/myapp_prod.sql

# 3. Verify extension works
psql myapp_prod_restored -c "SELECT pg_hello('Recovery Test');"

echo "Recovery completed successfully"
EOF
```

This deployment guide provides comprehensive coverage for getting your extension running in any environment, from development to enterprise production! 🚀

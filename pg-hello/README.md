# PostgreSQL Extension: pg_hello

A comprehensive example PostgreSQL extension written in C that demonstrates key concepts including custom functions, configuration parameters (GUCs), and Server Programming Interface (SPI) usage.

## 📋 Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Quick Start](#quick-start)
- [Architecture](#architecture)
- [Installation](#installation)
- [Usage Examples](#usage-examples)
- [Development](#development)
- [Additional Documentation](#additional-documentation)
- [Troubleshooting](#troubleshooting)

## 🔍 Overview

This extension provides three custom functions and demonstrates essential PostgreSQL extension development concepts:

- **Custom C Functions**: Native PostgreSQL functions written in C
- **GUC Parameters**: Custom configuration settings
- **SPI Integration**: Server-side SQL execution from C code
- **Extension Framework**: Proper packaging and installation

## ✨ Features

### Functions Provided

1. **`pg_hello(text)`** → `text`
   - Returns a customizable greeting message
   - Respects the `pg_hello.repeat` configuration parameter
   - Example: `SELECT pg_hello('World')` → `'Hello, World!'`

2. **`now_ms()`** → `bigint`
   - Returns current timestamp in milliseconds
   - Uses PostgreSQL's internal timestamp functions
   - Example: `SELECT now_ms()` → `1703123456789`

3. **`spi_version()`** → `text`
   - Demonstrates Server Programming Interface (SPI) usage
   - Executes `SELECT version()` internally and returns result
   - Example: Shows how C code can run SQL queries

### Configuration Parameters

- **`pg_hello.repeat`** (integer, range: 1-10, default: 1)
  - Controls how many times the greeting is repeated
  - Settable per session: `SET pg_hello.repeat = 3`

## 🚀 Quick Start

### Prerequisites

- PostgreSQL 12+ with development headers
- C compiler (gcc or clang)
- Make utility
- `pg_config` in your PATH

### Installation

```bash
# 1. Compile the extension
make clean
make

# 2. Install to PostgreSQL
sudo make install

# 3. Create extension in your database
psql -d your_database -c "CREATE EXTENSION pg_hello;"
```

### Basic Usage

```sql
-- Simple greeting
SELECT pg_hello('Developer');
-- Result: Hello, Developer!

-- Multiple greetings
SET pg_hello.repeat = 3;
SELECT pg_hello('PostgreSQL');
-- Result: Hello, PostgreSQL! Hello, PostgreSQL! Hello, PostgreSQL!

-- Current timestamp in milliseconds
SELECT now_ms();
-- Result: 1703123456789

-- PostgreSQL version via SPI
SELECT spi_version();
-- Result: PostgreSQL 15.13 (Homebrew) on aarch64-apple-darwin...
```

## 🏗️ Architecture

### File Structure

```
pg-hello/
├── src/
│   └── pg_hello.c          # Main C source code
├── sql/
│   └── pg_hello--1.0.sql   # SQL definition file
├── test/
│   ├── sql/
│   │   └── basic.sql       # Test SQL scripts
│   └── expected/
│       └── basic.out       # Expected test output
├── Makefile                # Build configuration
├── pg_hello.control        # Extension metadata
└── README.md              # This file
```

### Core Components

1. **C Source Code** (`src/pg_hello.c`)
   - Function implementations
   - PostgreSQL API integration
   - GUC parameter definitions

2. **SQL Definitions** (`sql/pg_hello--1.0.sql`)
   - Function declarations
   - SQL interface definitions

3. **Control File** (`pg_hello.control`)
   - Extension metadata
   - Version information
   - Dependencies

4. **Makefile**
   - Build rules and dependencies
   - Installation procedures
   - Test execution

## 📦 Installation

### Development Installation

```bash
# Clone or download the source
cd pg-hello/

# Compile
make clean && make

# Install (requires sudo for system-wide installation)
sudo make install

# Test installation
make installcheck
```

### Using the Extension

```sql
-- Connect to your database
psql -d your_database

-- Create the extension
CREATE EXTENSION pg_hello;

-- Verify installation
\dx pg_hello

-- Test functions
SELECT pg_hello('Test');
```

## 💡 Usage Examples

### Basic Function Calls

```sql
-- Simple greeting
SELECT pg_hello('Alice') AS greeting;

-- Timestamp operations
SELECT 
    now_ms() AS current_ms,
    extract(epoch from now()) * 1000 AS epoch_ms;

-- System information
SELECT spi_version() AS pg_version;
```

### Configuration Management

```sql
-- Check current setting
SHOW pg_hello.repeat;

-- Temporary change (session-level)
SET pg_hello.repeat = 5;
SELECT pg_hello('Repeated');

-- Reset to default
RESET pg_hello.repeat;
```

### Advanced Usage

```sql
-- Using in queries
SELECT 
    name,
    pg_hello(name) AS personalized_greeting
FROM users
LIMIT 5;

-- Performance timing
SELECT 
    now_ms() AS start_time,
    pg_sleep(1),
    now_ms() AS end_time,
    (now_ms() - (SELECT now_ms() - 1000)) AS elapsed_ms;
```

## 🔧 Development

### Building from Source

```bash
# Ensure PostgreSQL development headers are installed
# Ubuntu/Debian: sudo apt-get install postgresql-server-dev-all
# CentOS/RHEL: sudo yum install postgresql-devel
# macOS: brew install postgresql

# Compile
make clean
make

# Install locally
make install

# Run tests
make installcheck
```

### Modifying the Extension

1. **Adding New Functions**:
   - Add C function to `src/pg_hello.c`
   - Add SQL declaration to `sql/pg_hello--1.0.sql`
   - Rebuild and reinstall

2. **Adding Configuration Parameters**:
   - Define variable in `_PG_init()`
   - Use `DefineCustomIntVariable()` or similar
   - Reference in your functions

3. **Version Updates**:
   - Create new SQL file: `sql/pg_hello--1.1.sql`
   - Update `pg_hello.control` default_version
   - Provide upgrade path if needed

### Code Style Guidelines

- Follow PostgreSQL coding conventions
- Use PostgreSQL memory contexts
- Handle errors with `ereport()`
- Always clean up resources (SPI_finish(), etc.)
- Declare variables at function start (C90 compatibility)

## 📚 Additional Documentation

- [Makefile Documentation](docs/MAKEFILE.md) - Understanding the build system
- [C Code Structure](docs/C_CODE.md) - Deep dive into the C implementation
- [PostgreSQL Extension System](docs/EXTENSION_SYSTEM.md) - How extensions work
- [Build Process](docs/BUILD_PROCESS.md) - Detailed build and installation guide

## 🐛 Troubleshooting

### Common Issues

1. **`pg_config not found`**
   ```bash
   # Add PostgreSQL bin to PATH
   export PATH=/usr/pgsql-15/bin:$PATH
   ```

2. **Permission denied during installation**
   ```bash
   # Use sudo for system installation
   sudo make install
   ```

3. **Extension already exists**
   ```sql
   -- Drop and recreate
   DROP EXTENSION IF EXISTS pg_hello;
   CREATE EXTENSION pg_hello;
   ```

4. **Function not found after installation**
   ```sql
   -- Verify extension is loaded
   \dx
   
   -- Check specific extension
   \dx+ pg_hello
   ```

### Build Errors

1. **Missing PostgreSQL headers**
   - Install postgresql-server-dev package
   - Verify pg_config works: `pg_config --includedir`

2. **Compiler warnings about C90/C99**
   - Variables must be declared at function start
   - No mixed declarations and code

3. **MODULE_PATHNAME errors**
   - Ensure SQL file uses `$libdir/pg_hello`
   - Not `MODULE_PATHNAME` literal

### Debugging

```bash
# Verbose compilation
make CFLAGS="-g -O0 -Wall"

# Check shared library
ldd pg_hello.so

# PostgreSQL logs
tail -f /var/log/postgresql/postgresql-15-main.log
```

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Add tests
5. Submit a pull request

## 📄 License

This is educational example code. Use and modify as needed for your projects.

## 🙏 Acknowledgments

- PostgreSQL Development Team for excellent documentation
- Community examples and tutorials
- PostgreSQL Extension Building Infrastructure (PGXS)

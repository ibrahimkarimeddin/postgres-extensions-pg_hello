# Quick Reference Guide

Fast reference for common tasks, commands, and troubleshooting with the `pg_hello` extension.

## 📋 Quick Commands

### Installation & Setup
```bash
# Build and install
make clean && make && sudo make install

# Create extension in database
psql -d mydb -c "CREATE EXTENSION pg_hello;"

# Test installation
psql -d mydb -c "SELECT pg_hello('Test');"
```

### Basic Usage
```sql
-- Simple greeting
SELECT pg_hello('World');
-- Result: Hello, World!

-- Configure repeat count
SET pg_hello.repeat = 3;
SELECT pg_hello('PostgreSQL');
-- Result: Hello, PostgreSQL! Hello, PostgreSQL! Hello, PostgreSQL!

-- Get current timestamp in milliseconds
SELECT now_ms();
-- Result: 1703123456789

-- Get PostgreSQL version info
SELECT spi_version();
-- Result: PostgreSQL 15.13 (Homebrew) on aarch64-apple-darwin...
```

### Extension Management
```sql
-- List all extensions
\dx

-- Extension details
\dx+ pg_hello

-- Drop extension
DROP EXTENSION pg_hello CASCADE;

-- Reinstall
CREATE EXTENSION pg_hello;
```

## 🔧 Configuration Quick Reference

### GUC Parameters
| Parameter | Type | Range | Default | Description |
|-----------|------|-------|---------|-------------|
| `pg_hello.repeat` | integer | 1-10 | 1 | Number of times to repeat greeting |

### Configuration Commands
```sql
-- View current setting
SHOW pg_hello.repeat;

-- Set for session
SET pg_hello.repeat = 5;

-- Reset to default
RESET pg_hello.repeat;

-- Set globally (requires superuser)
ALTER SYSTEM SET pg_hello.repeat = 2;
SELECT pg_reload_conf();
```

## 📊 Monitoring Quick Reference

### Function Statistics
```sql
-- View function call statistics
SELECT 
    funcname,
    calls,
    total_time,
    mean_time
FROM pg_stat_user_functions 
WHERE funcname LIKE 'pg_hello%' OR funcname IN ('now_ms', 'spi_version');

-- Reset statistics
SELECT pg_stat_reset();
```

### Health Check
```sql
-- Extension health check
SELECT 
    extname,
    extversion,
    (extname IS NOT NULL) as is_installed
FROM pg_extension 
WHERE extname = 'pg_hello';

-- Function availability check
\df pg_hello*

-- Test all functions
SELECT 
    'pg_hello' as function_name,
    pg_hello('Health Check') as result
UNION ALL
SELECT 
    'now_ms' as function_name,
    now_ms()::text as result
UNION ALL
SELECT 
    'spi_version' as function_name,
    spi_version() as result;
```

## 🚨 Troubleshooting Quick Fixes

### Common Errors & Solutions

#### "extension does not exist"
```bash
# Check if extension is available
psql -c "SELECT * FROM pg_available_extensions WHERE name = 'pg_hello';"

# If not available, reinstall
make clean && make && sudo make install
```

#### "could not load library"
```bash
# Check if library file exists
ls -la $(pg_config --libdir)/pg_hello.so

# Check library dependencies
ldd $(pg_config --libdir)/pg_hello.so  # Linux
otool -L $(pg_config --libdir)/pg_hello.so  # macOS

# Reinstall if missing
sudo make install
```

#### "function does not exist"
```sql
-- Check if extension is created
\dx pg_hello

-- If not created
CREATE EXTENSION pg_hello;

-- If created but functions missing, recreate
DROP EXTENSION pg_hello CASCADE;
CREATE EXTENSION pg_hello;
```

#### "permission denied"
```bash
# Installation permission issues
sudo make install

# Database permission issues
psql -c "CREATE EXTENSION pg_hello;" # Run as superuser
```

### Performance Issues
```sql
-- Check for slow queries
SELECT 
    funcname,
    calls,
    mean_time,
    total_time
FROM pg_stat_user_functions 
WHERE mean_time > 10  -- Functions taking >10ms
AND funcname IN ('pg_hello', 'now_ms', 'spi_version');

-- If slow, reset stats and monitor
SELECT pg_stat_reset();
```

## 📁 File Locations Quick Reference

### Extension Files
```bash
# Control file
$(pg_config --sharedir)/extension/pg_hello.control

# SQL definition files
$(pg_config --sharedir)/extension/pg_hello--*.sql

# Shared library
$(pg_config --libdir)/pg_hello.so

# Source code (development)
./src/pg_hello.c
./Makefile
```

### Log Files
```bash
# PostgreSQL logs
/var/log/postgresql/postgresql-15-main.log  # Ubuntu/Debian
/var/lib/pgsql/15/data/log/postgresql-*.log  # CentOS/RHEL
$(brew --prefix)/var/log/postgresql@15.log   # macOS Homebrew
```

## 🔍 Debugging Quick Reference

### Enable Debug Logging
```sql
-- Enable detailed logging
SET log_statement = 'all';
SET log_min_messages = 'debug1';

-- Test function
SELECT pg_hello('Debug Test');

-- Check logs for details
```

### Function Call Tracing
```bash
# Linux: Trace library loading
sudo strace -p $(pgrep postgres) -e trace=openat,mmap -f

# macOS: Use dtrace or Console.app
sudo dtruss -p $(pgrep postgres) -t open
```

### Memory Debugging
```sql
-- Check memory contexts (PostgreSQL 14+)
SELECT * FROM pg_backend_memory_contexts 
WHERE name LIKE '%hello%';

-- Check for memory leaks
SELECT pg_size_pretty(pg_total_relation_size('pg_proc'));
```

## 🔄 Development Workflow

### Daily Development Cycle
```bash
# 1. Edit source code
vim src/pg_hello.c

# 2. Build
make clean && make

# 3. Install
sudo make install

# 4. Test in database
psql mydb -c "
  DROP EXTENSION IF EXISTS pg_hello CASCADE;
  CREATE EXTENSION pg_hello;
  SELECT pg_hello('Development Test');
"

# 5. Run tests
make installcheck
```

### Version Update Process
```bash
# 1. Create new SQL file
cp sql/pg_hello--1.0.sql sql/pg_hello--1.1.sql

# 2. Create upgrade script
cat > sql/pg_hello--1.0--1.1.sql << 'EOF'
-- Upgrade commands here
EOF

# 3. Update control file
sed -i 's/default_version = .*/default_version = '\''1.1'\''/' pg_hello.control

# 4. Build and install
make clean && make && sudo make install

# 5. Test upgrade
psql -c "ALTER EXTENSION pg_hello UPDATE TO '1.1';"
```

## 📋 Checklists

### Pre-Production Checklist
- [ ] Extension compiles without warnings
- [ ] All tests pass (`make installcheck`)
- [ ] Performance is acceptable
- [ ] Documentation is updated
- [ ] Version is tagged in git
- [ ] Backup procedures tested

### Deployment Checklist
- [ ] Extension installed on all servers
- [ ] Database extensions created
- [ ] Function tests completed
- [ ] Monitoring configured
- [ ] Rollback plan ready

### Troubleshooting Checklist
- [ ] Check PostgreSQL logs
- [ ] Verify extension files exist
- [ ] Test function availability
- [ ] Check permissions
- [ ] Validate configuration
- [ ] Monitor performance

## 🔗 Useful SQL Queries

### Extension Information
```sql
-- Complete extension overview
SELECT 
    e.extname,
    e.extversion,
    n.nspname as schema,
    e.extrelocatable,
    d.description
FROM pg_extension e
JOIN pg_namespace n ON n.oid = e.extnamespace
LEFT JOIN pg_description d ON d.objoid = e.oid
WHERE e.extname = 'pg_hello';
```

### Function Details
```sql
-- Detailed function information
SELECT 
    p.proname,
    pg_get_function_identity_arguments(p.oid) as arguments,
    format_type(p.prorettype, NULL) as return_type,
    p.provolatile,
    p.proisstrict,
    p.prosecdef,
    p.probin as library,
    p.prosrc as symbol
FROM pg_proc p
WHERE p.proname LIKE 'pg_hello%' 
   OR p.proname IN ('now_ms', 'spi_version');
```

### Performance Analysis
```sql
-- Performance summary
WITH function_stats AS (
    SELECT 
        funcname,
        calls,
        total_time,
        mean_time,
        stddev_time
    FROM pg_stat_user_functions
    WHERE funcname IN ('pg_hello', 'now_ms', 'spi_version')
)
SELECT 
    funcname,
    calls,
    ROUND(mean_time::numeric, 3) as avg_ms,
    ROUND(total_time::numeric, 3) as total_ms,
    CASE 
        WHEN mean_time < 1 THEN 'Excellent'
        WHEN mean_time < 5 THEN 'Good'
        WHEN mean_time < 10 THEN 'Acceptable'
        ELSE 'Needs attention'
    END as performance_rating
FROM function_stats
ORDER BY mean_time DESC;
```

This quick reference should help you work efficiently with the `pg_hello` extension! 📖

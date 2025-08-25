# Operations and Maintenance Guide

Complete guide for operating, monitoring, and maintaining the `pg_hello` extension in production environments.

## 📋 Table of Contents

- [Operations Overview](#operations-overview)
- [Monitoring and Observability](#monitoring-and-observability)
- [Performance Management](#performance-management)
- [Backup and Recovery](#backup-and-recovery)
- [Security Operations](#security-operations)
- [Troubleshooting](#troubleshooting)
- [Maintenance Procedures](#maintenance-procedures)
- [Disaster Recovery](#disaster-recovery)
- [Compliance and Auditing](#compliance-and-auditing)

## 🔍 Operations Overview

### Operational Responsibilities

| Area | Responsibility | Frequency | Tools |
|------|---------------|-----------|-------|
| **Monitoring** | Track extension health and performance | Continuous | Grafana, Prometheus |
| **Backup** | Ensure extension data is backed up | Daily | pg_dump, Barman |
| **Updates** | Apply extension updates safely | As needed | Blue-green deployment |
| **Security** | Monitor for security issues | Continuous | Log analysis, SIEM |
| **Performance** | Optimize extension performance | Weekly | pg_stat_statements |
| **Documentation** | Keep operational docs updated | Monthly | Confluence, Wiki |

### Service Level Objectives (SLOs)

```yaml
# Extension SLOs
extension_availability:
  target: 99.9%
  measurement: Extension functions respond successfully
  
function_latency:
  target: 95th percentile < 10ms
  measurement: pg_hello() function execution time
  
error_rate:
  target: < 0.1%
  measurement: Failed extension function calls
  
backup_recovery:
  target: RTO < 4 hours, RPO < 1 hour
  measurement: Time to restore extension functionality
```

## 📊 Monitoring and Observability

### PostgreSQL Metrics

```sql
-- Create monitoring views for pg_hello extension
CREATE VIEW pg_hello_stats AS
SELECT 
    schemaname,
    funcname,
    calls,
    total_time,
    mean_time,
    stddev_time,
    (total_time / calls) as avg_time_ms
FROM pg_stat_user_functions 
WHERE funcname LIKE 'pg_hello%'
OR funcname IN ('now_ms', 'spi_version');

-- Monitor extension usage
CREATE VIEW pg_hello_usage AS
SELECT 
    funcname,
    calls,
    total_time,
    ROUND((total_time / calls)::numeric, 3) as avg_time_ms,
    ROUND((100.0 * calls / SUM(calls) OVER())::numeric, 2) as call_percentage
FROM pg_stat_user_functions 
WHERE funcname IN ('pg_hello', 'now_ms', 'spi_version')
ORDER BY calls DESC;

-- Check extension configuration
CREATE VIEW pg_hello_config AS
SELECT 
    name,
    setting,
    unit,
    context,
    source
FROM pg_settings 
WHERE name LIKE 'pg_hello%';
```

### Prometheus Metrics

```yaml
# prometheus-postgres-exporter.yml
# Configuration for postgres_exporter to monitor pg_hello

pg_hello_function_calls:
  query: |
    SELECT 
      funcname as function_name,
      calls,
      total_time,
      mean_time
    FROM pg_stat_user_functions 
    WHERE funcname IN ('pg_hello', 'now_ms', 'spi_version')
  metrics:
    - function_name:
        usage: "LABEL"
        description: "Function name"
    - calls:
        usage: "COUNTER"
        description: "Number of times function has been called"
    - total_time:
        usage: "COUNTER"
        description: "Total time spent in function (ms)"
    - mean_time:
        usage: "GAUGE"
        description: "Mean time per function call (ms)"

pg_hello_extension_info:
  query: |
    SELECT 
      extname as extension_name,
      extversion as version,
      1 as installed
    FROM pg_extension 
    WHERE extname = 'pg_hello'
  metrics:
    - extension_name:
        usage: "LABEL"
        description: "Extension name"
    - version:
        usage: "LABEL"
        description: "Extension version"
    - installed:
        usage: "GAUGE"
        description: "Extension installation status"
```

### Grafana Dashboard

```json
{
  "dashboard": {
    "title": "PostgreSQL pg_hello Extension",
    "panels": [
      {
        "title": "Function Call Rate",
        "type": "graph",
        "targets": [
          {
            "expr": "rate(pg_hello_function_calls[5m])",
            "legendFormat": "{{ function_name }}"
          }
        ]
      },
      {
        "title": "Function Latency",
        "type": "graph",
        "targets": [
          {
            "expr": "pg_hello_function_calls_mean_time",
            "legendFormat": "{{ function_name }} avg latency"
          }
        ]
      },
      {
        "title": "Extension Status",
        "type": "stat",
        "targets": [
          {
            "expr": "pg_hello_extension_info",
            "legendFormat": "v{{ version }}"
          }
        ]
      }
    ]
  }
}
```

### Health Check Scripts

```bash
#!/bin/bash
# health_check.sh - Comprehensive health check for pg_hello extension

DATABASES=("production" "staging" "analytics")
ALERT_EMAIL="ops@company.com"
LOG_FILE="/var/log/pg_hello_health.log"

log_message() {
    echo "$(date '+%Y-%m-%d %H:%M:%S') - $1" | tee -a "$LOG_FILE"
}

check_extension_installed() {
    local db=$1
    local result=$(psql -d "$db" -t -c "SELECT COUNT(*) FROM pg_extension WHERE extname = 'pg_hello';" 2>/dev/null)
    
    if [ "$result" = "1" ]; then
        log_message "✓ Extension installed in $db"
        return 0
    else
        log_message "✗ Extension NOT installed in $db"
        return 1
    fi
}

check_functions_work() {
    local db=$1
    local errors=0
    
    # Test pg_hello function
    if ! psql -d "$db" -c "SELECT pg_hello('Health Check');" >/dev/null 2>&1; then
        log_message "✗ pg_hello function failed in $db"
        ((errors++))
    fi
    
    # Test now_ms function
    if ! psql -d "$db" -c "SELECT now_ms();" >/dev/null 2>&1; then
        log_message "✗ now_ms function failed in $db"
        ((errors++))
    fi
    
    # Test spi_version function
    if ! psql -d "$db" -c "SELECT spi_version();" >/dev/null 2>&1; then
        log_message "✗ spi_version function failed in $db"
        ((errors++))
    fi
    
    if [ $errors -eq 0 ]; then
        log_message "✓ All functions working in $db"
        return 0
    else
        log_message "✗ $errors function(s) failed in $db"
        return 1
    fi
}

check_performance() {
    local db=$1
    
    # Test performance with 100 calls
    local start_time=$(date +%s%N)
    psql -d "$db" -c "SELECT pg_hello('Performance Test ' || i::text) FROM generate_series(1, 100) i;" >/dev/null 2>&1
    local end_time=$(date +%s%N)
    
    local duration=$(( (end_time - start_time) / 1000000 )) # Convert to milliseconds
    local avg_per_call=$(( duration / 100 ))
    
    if [ $avg_per_call -lt 10 ]; then
        log_message "✓ Performance OK in $db (${avg_per_call}ms per call)"
        return 0
    else
        log_message "⚠ Performance degraded in $db (${avg_per_call}ms per call)"
        return 1
    fi
}

send_alert() {
    local message=$1
    echo "$message" | mail -s "pg_hello Extension Alert" "$ALERT_EMAIL"
}

main() {
    log_message "Starting pg_hello health check"
    local total_errors=0
    
    for db in "${DATABASES[@]}"; do
        log_message "Checking database: $db"
        
        if ! check_extension_installed "$db"; then
            ((total_errors++))
            send_alert "pg_hello extension not installed in database: $db"
        elif ! check_functions_work "$db"; then
            ((total_errors++))
            send_alert "pg_hello functions not working in database: $db"
        elif ! check_performance "$db"; then
            # Performance issue is warning, not error
            send_alert "pg_hello performance degraded in database: $db"
        fi
    done
    
    if [ $total_errors -eq 0 ]; then
        log_message "✓ All health checks passed"
        exit 0
    else
        log_message "✗ $total_errors error(s) found"
        exit 1
    fi
}

main "$@"
```

### Log Monitoring

```bash
# log_monitor.sh - Monitor PostgreSQL logs for pg_hello issues
#!/bin/bash

LOG_FILE="/var/log/postgresql/postgresql.log"
ALERT_PATTERNS=(
    "ERROR.*pg_hello"
    "FATAL.*pg_hello"
    "could not load.*pg_hello"
    "extension.*pg_hello.*failed"
)

monitor_logs() {
    tail -f "$LOG_FILE" | while read line; do
        for pattern in "${ALERT_PATTERNS[@]}"; do
            if echo "$line" | grep -E "$pattern" >/dev/null; then
                echo "ALERT: pg_hello issue detected: $line"
                echo "$line" | mail -s "pg_hello Extension Error" ops@company.com
            fi
        done
    done
}

# Run log monitoring
monitor_logs
```

## ⚡ Performance Management

### Performance Monitoring Queries

```sql
-- Monitor function performance over time
CREATE OR REPLACE FUNCTION pg_hello_performance_report()
RETURNS TABLE(
    function_name text,
    calls_per_second numeric,
    avg_time_ms numeric,
    total_calls bigint,
    performance_trend text
) AS $$
BEGIN
    RETURN QUERY
    WITH current_stats AS (
        SELECT 
            funcname,
            calls,
            mean_time,
            total_time
        FROM pg_stat_user_functions 
        WHERE funcname IN ('pg_hello', 'now_ms', 'spi_version')
    ),
    rates AS (
        SELECT 
            funcname,
            calls / EXTRACT(EPOCH FROM (now() - pg_postmaster_start_time())) as cps,
            mean_time,
            calls,
            CASE 
                WHEN mean_time < 5 THEN 'Excellent'
                WHEN mean_time < 10 THEN 'Good'
                WHEN mean_time < 20 THEN 'Acceptable'
                ELSE 'Poor'
            END as trend
        FROM current_stats
    )
    SELECT 
        r.funcname::text,
        ROUND(r.cps, 2),
        ROUND(r.mean_time, 3),
        r.calls,
        r.trend::text
    FROM rates r
    ORDER BY r.cps DESC;
END;
$$ LANGUAGE plpgsql;

-- Performance baseline capture
CREATE TABLE pg_hello_performance_baseline (
    recorded_at timestamp with time zone DEFAULT now(),
    function_name text,
    calls_total bigint,
    mean_time_ms numeric,
    calls_per_second numeric
);

-- Capture baseline
INSERT INTO pg_hello_performance_baseline (function_name, calls_total, mean_time_ms, calls_per_second)
SELECT * FROM pg_hello_performance_report();
```

### Performance Optimization

```sql
-- Optimize PostgreSQL settings for pg_hello extension
-- Add to postgresql.conf

# Memory settings
shared_buffers = 256MB                  # Increase for better caching
work_mem = 4MB                         # Memory for operations
maintenance_work_mem = 64MB            # Memory for maintenance

# Query optimization
random_page_cost = 1.1                 # SSD optimization
effective_cache_size = 1GB             # OS cache size
default_statistics_target = 100        # Statistics accuracy

# Connection settings
max_connections = 100                   # Adjust based on load
shared_preload_libraries = 'pg_stat_statements'  # Track query performance

# Extension-specific optimization
pg_hello.repeat = 1                    # Default configuration
```

### Load Testing

```bash
#!/bin/bash
# load_test.sh - Load testing for pg_hello extension

CONCURRENT_USERS=10
DURATION=60  # seconds
DATABASE="production"

# Function to simulate user load
simulate_user() {
    local user_id=$1
    local end_time=$(($(date +%s) + DURATION))
    
    while [ $(date +%s) -lt $end_time ]; do
        # Random greeting test
        psql -d "$DATABASE" -c "SELECT pg_hello('User${user_id}_$(date +%s)');" >/dev/null 2>&1
        
        # Configuration change test
        local repeat=$((RANDOM % 5 + 1))
        psql -d "$DATABASE" -c "SET pg_hello.repeat = $repeat; SELECT pg_hello('Load Test');" >/dev/null 2>&1
        
        # Timestamp test
        psql -d "$DATABASE" -c "SELECT now_ms();" >/dev/null 2>&1
        
        sleep 0.1  # 100ms between requests
    done
}

echo "Starting load test with $CONCURRENT_USERS concurrent users for ${DURATION}s"

# Capture baseline performance
psql -d "$DATABASE" -c "SELECT pg_stat_reset();"
baseline=$(psql -d "$DATABASE" -t -c "SELECT now();")

# Start concurrent users
for i in $(seq 1 $CONCURRENT_USERS); do
    simulate_user $i &
done

# Wait for all users to complete
wait

# Capture results
echo "Load test completed. Performance results:"
psql -d "$DATABASE" -c "SELECT * FROM pg_hello_performance_report();"
```

## 💾 Backup and Recovery

### Extension-Aware Backup Strategy

```bash
#!/bin/bash
# backup_pg_hello.sh - Comprehensive backup including extension

BACKUP_BASE="/backup/pg_hello"
DATE=$(date +%Y%m%d_%H%M%S)
BACKUP_DIR="$BACKUP_BASE/$DATE"
DATABASES=("production" "staging")

mkdir -p "$BACKUP_DIR"

backup_extension_files() {
    echo "Backing up extension files..."
    
    # Control file
    cp "$(pg_config --sharedir)/extension/pg_hello.control" "$BACKUP_DIR/"
    
    # SQL files
    cp "$(pg_config --sharedir)/extension/pg_hello"*.sql "$BACKUP_DIR/"
    
    # Shared library
    cp "$(pg_config --libdir)/pg_hello.so" "$BACKUP_DIR/"
    
    # Create manifest
    cat > "$BACKUP_DIR/extension_manifest.txt" << EOF
Extension: pg_hello
Backup Date: $(date)
PostgreSQL Version: $(pg_config --version)
Extension Files:
$(ls -la "$BACKUP_DIR/pg_hello"*)
EOF
}

backup_database() {
    local db=$1
    echo "Backing up database: $db"
    
    # Full database dump
    pg_dump "$db" > "$BACKUP_DIR/${db}_full.sql"
    
    # Extension-specific data
    pg_dump "$db" \
        --table="*pg_hello*" \
        --table="app_config" \
        > "$BACKUP_DIR/${db}_extension_data.sql"
    
    # Extension metadata
    pg_dump "$db" \
        --schema-only \
        --extension=pg_hello \
        > "$BACKUP_DIR/${db}_extension_schema.sql"
    
    # Configuration dump
    psql -d "$db" -c "
        SELECT name, setting, unit, context 
        FROM pg_settings 
        WHERE name LIKE 'pg_hello%'
    " > "$BACKUP_DIR/${db}_config.txt"
}

verify_backup() {
    echo "Verifying backup integrity..."
    
    for db in "${DATABASES[@]}"; do
        if [ ! -f "$BACKUP_DIR/${db}_full.sql" ]; then
            echo "ERROR: Missing backup for database $db"
            exit 1
        fi
        
        # Test SQL syntax
        if ! pg_dump --schema-only "$db" | psql template1 -f - >/dev/null 2>&1; then
            echo "WARNING: Backup may have syntax issues for $db"
        fi
    done
    
    echo "Backup verification completed"
}

compress_backup() {
    echo "Compressing backup..."
    cd "$BACKUP_BASE"
    tar -czf "${DATE}.tar.gz" "$DATE/"
    
    if [ $? -eq 0 ]; then
        rm -rf "$DATE/"
        echo "Backup compressed to ${DATE}.tar.gz"
    else
        echo "ERROR: Compression failed"
        exit 1
    fi
}

cleanup_old_backups() {
    echo "Cleaning up old backups (keeping last 7 days)..."
    find "$BACKUP_BASE" -name "*.tar.gz" -mtime +7 -delete
}

main() {
    echo "Starting pg_hello backup at $(date)"
    
    backup_extension_files
    
    for db in "${DATABASES[@]}"; do
        backup_database "$db"
    done
    
    verify_backup
    compress_backup
    cleanup_old_backups
    
    echo "Backup completed successfully: $BACKUP_BASE/${DATE}.tar.gz"
}

main "$@"
```

### Recovery Procedures

```bash
#!/bin/bash
# restore_pg_hello.sh - Recovery script for pg_hello extension

BACKUP_FILE=$1
RESTORE_DB=$2

if [ -z "$BACKUP_FILE" ] || [ -z "$RESTORE_DB" ]; then
    echo "Usage: $0 <backup_file.tar.gz> <target_database>"
    exit 1
fi

TEMP_DIR="/tmp/pg_hello_restore_$$"
mkdir -p "$TEMP_DIR"

extract_backup() {
    echo "Extracting backup..."
    tar -xzf "$BACKUP_FILE" -C "$TEMP_DIR"
    
    # Find the backup directory
    BACKUP_DIR=$(find "$TEMP_DIR" -type d -name "20*" | head -1)
    
    if [ ! -d "$BACKUP_DIR" ]; then
        echo "ERROR: Could not find backup directory"
        exit 1
    fi
    
    echo "Backup extracted to: $BACKUP_DIR"
}

restore_extension_files() {
    echo "Restoring extension files..."
    
    # Restore control file
    sudo cp "$BACKUP_DIR/pg_hello.control" "$(pg_config --sharedir)/extension/"
    
    # Restore SQL files
    sudo cp "$BACKUP_DIR"/pg_hello*.sql "$(pg_config --sharedir)/extension/"
    
    # Restore shared library
    sudo cp "$BACKUP_DIR/pg_hello.so" "$(pg_config --libdir)/"
    
    # Set correct permissions
    sudo chown postgres:postgres "$(pg_config --sharedir)/extension/pg_hello"*
    sudo chown postgres:postgres "$(pg_config --libdir)/pg_hello.so"
    sudo chmod 644 "$(pg_config --sharedir)/extension/pg_hello"*
    sudo chmod 755 "$(pg_config --libdir)/pg_hello.so"
}

restore_database() {
    echo "Restoring database: $RESTORE_DB"
    
    # Create database if it doesn't exist
    createdb "$RESTORE_DB" 2>/dev/null || true
    
    # Restore full database
    psql "$RESTORE_DB" < "$BACKUP_DIR/${RESTORE_DB}_full.sql"
    
    if [ $? -eq 0 ]; then
        echo "Database restored successfully"
    else
        echo "ERROR: Database restore failed"
        exit 1
    fi
}

verify_restoration() {
    echo "Verifying restoration..."
    
    # Check extension is installed
    local ext_count=$(psql -d "$RESTORE_DB" -t -c "SELECT COUNT(*) FROM pg_extension WHERE extname = 'pg_hello';")
    if [ "$ext_count" != "1" ]; then
        echo "ERROR: Extension not properly restored"
        exit 1
    fi
    
    # Test functions
    psql -d "$RESTORE_DB" -c "SELECT pg_hello('Restore Test');" >/dev/null 2>&1
    if [ $? -ne 0 ]; then
        echo "ERROR: Extension functions not working"
        exit 1
    fi
    
    echo "Restoration verified successfully"
}

cleanup() {
    echo "Cleaning up temporary files..."
    rm -rf "$TEMP_DIR"
}

main() {
    echo "Starting pg_hello restoration from $BACKUP_FILE to $RESTORE_DB"
    
    extract_backup
    restore_extension_files
    restore_database
    verify_restoration
    cleanup
    
    echo "Restoration completed successfully"
}

# Set up cleanup on exit
trap cleanup EXIT

main "$@"
```

## 🔒 Security Operations

### Security Monitoring

```sql
-- Monitor extension access patterns
CREATE VIEW pg_hello_access_audit AS
SELECT 
    usename,
    datname,
    query,
    query_start,
    state,
    client_addr
FROM pg_stat_activity 
WHERE query ILIKE '%pg_hello%'
   OR query ILIKE '%now_ms%'
   OR query ILIKE '%spi_version%';

-- Log suspicious activity
CREATE OR REPLACE FUNCTION log_extension_usage()
RETURNS TRIGGER AS $$
BEGIN
    -- Log all extension function calls with metadata
    INSERT INTO security_audit_log (
        timestamp,
        username,
        database_name,
        function_called,
        parameters,
        client_ip
    ) VALUES (
        now(),
        session_user,
        current_database(),
        TG_TABLE_NAME,
        NEW::text,
        inet_client_addr()
    );
    
    RETURN NEW;
END;
$$ LANGUAGE plpgsql;
```

### Access Control

```sql
-- Create security roles for pg_hello extension
CREATE ROLE pg_hello_users;
CREATE ROLE pg_hello_admins;

-- Grant appropriate permissions
GRANT EXECUTE ON FUNCTION pg_hello(text) TO pg_hello_users;
GRANT EXECUTE ON FUNCTION now_ms() TO pg_hello_users;
GRANT EXECUTE ON FUNCTION spi_version() TO pg_hello_admins;

-- Restrict configuration changes
REVOKE ALL ON FUNCTION update_greeting_config(integer) FROM PUBLIC;
GRANT EXECUTE ON FUNCTION update_greeting_config(integer) TO pg_hello_admins;

-- Row Level Security for extension data
ALTER TABLE app_config ENABLE ROW LEVEL SECURITY;

CREATE POLICY pg_hello_config_policy ON app_config
    FOR ALL TO pg_hello_users
    USING (key LIKE 'pg_hello%' AND current_user = 'app_user');
```

### Security Hardening

```bash
#!/bin/bash
# security_hardening.sh - Harden pg_hello extension security

echo "Applying security hardening for pg_hello extension..."

# File permissions
echo "Setting secure file permissions..."
sudo chmod 644 "$(pg_config --sharedir)/extension/pg_hello"*
sudo chmod 755 "$(pg_config --libdir)/pg_hello.so"
sudo chown postgres:postgres "$(pg_config --sharedir)/extension/pg_hello"*
sudo chown postgres:postgres "$(pg_config --libdir)/pg_hello.so"

# Database security
echo "Applying database security settings..."
psql -c "
-- Restrict extension creation to superusers only
ALTER SYSTEM SET shared_preload_libraries = 'pg_hello';

-- Enable logging for extension functions
ALTER SYSTEM SET log_statement = 'mod';
ALTER SYSTEM SET log_min_duration_statement = 0;

-- Log all extension-related activities
ALTER SYSTEM SET log_line_prefix = '%t [%p]: [%l-1] user=%u,db=%d,app=%a,client=%h ';
"

# Restart PostgreSQL to apply settings
sudo systemctl restart postgresql

echo "Security hardening completed"
```

## 🔧 Troubleshooting

### Common Issues and Solutions

#### Issue 1: Extension Functions Not Working

```bash
# Diagnosis script
diagnose_extension_functions() {
    echo "=== Extension Function Diagnosis ==="
    
    # Check if extension is installed
    echo "1. Checking extension installation..."
    psql -c "\dx pg_hello"
    
    # Check if functions exist
    echo "2. Checking function definitions..."
    psql -c "\df pg_hello*"
    
    # Check shared library
    echo "3. Checking shared library..."
    ls -la "$(pg_config --libdir)/pg_hello.so"
    ldd "$(pg_config --libdir)/pg_hello.so" 2>/dev/null || otool -L "$(pg_config --libdir)/pg_hello.so"
    
    # Test function calls
    echo "4. Testing function calls..."
    psql -c "SELECT pg_hello('Test');" 2>&1
    psql -c "SELECT now_ms();" 2>&1
    psql -c "SELECT spi_version();" 2>&1
}
```

#### Issue 2: Performance Degradation

```sql
-- Performance diagnosis queries
-- Check for blocking queries
SELECT 
    blocked_locks.pid AS blocked_pid,
    blocked_activity.usename AS blocked_user,
    blocking_locks.pid AS blocking_pid,
    blocking_activity.usename AS blocking_user,
    blocked_activity.query AS blocked_statement,
    blocking_activity.query AS current_statement_in_blocking_process
FROM pg_catalog.pg_locks blocked_locks
JOIN pg_catalog.pg_stat_activity blocked_activity ON blocked_activity.pid = blocked_locks.pid
JOIN pg_catalog.pg_locks blocking_locks ON blocking_locks.locktype = blocked_locks.locktype
    AND blocking_locks.database IS NOT DISTINCT FROM blocked_locks.database
    AND blocking_locks.relation IS NOT DISTINCT FROM blocked_locks.relation
    AND blocking_locks.page IS NOT DISTINCT FROM blocked_locks.page
    AND blocking_locks.tuple IS NOT DISTINCT FROM blocked_locks.tuple
    AND blocking_locks.virtualxid IS NOT DISTINCT FROM blocked_locks.virtualxid
    AND blocking_locks.transactionid IS NOT DISTINCT FROM blocked_locks.transactionid
    AND blocking_locks.classid IS NOT DISTINCT FROM blocked_locks.classid
    AND blocking_locks.objid IS NOT DISTINCT FROM blocked_locks.objid
    AND blocking_locks.objsubid IS NOT DISTINCT FROM blocked_locks.objsubid
    AND blocking_locks.pid != blocked_locks.pid
JOIN pg_catalog.pg_stat_activity blocking_activity ON blocking_activity.pid = blocking_locks.pid
WHERE NOT blocked_locks.granted
    AND (blocked_activity.query ILIKE '%pg_hello%' OR blocking_activity.query ILIKE '%pg_hello%');

-- Check extension function statistics
SELECT 
    funcname,
    calls,
    total_time,
    mean_time,
    stddev_time
FROM pg_stat_user_functions 
WHERE funcname IN ('pg_hello', 'now_ms', 'spi_version')
ORDER BY mean_time DESC;
```

#### Issue 3: Memory Leaks

```bash
# Memory monitoring script
monitor_memory_usage() {
    echo "=== Memory Usage Monitoring ==="
    
    # PostgreSQL process memory
    echo "PostgreSQL process memory usage:"
    ps aux | grep postgres | awk '{print $1, $2, $4, $6, $11}' | column -t
    
    # Check for memory leaks in extension
    echo "Checking for memory leaks..."
    psql -c "
        SELECT 
            pg_size_pretty(pg_total_relation_size('pg_proc')) as proc_table_size,
            COUNT(*) as total_functions,
            COUNT(*) FILTER (WHERE proname LIKE 'pg_hello%') as extension_functions
        FROM pg_proc;
    "
    
    # Monitor memory contexts
    psql -c "SELECT * FROM pg_backend_memory_contexts WHERE name LIKE '%hello%';" 2>/dev/null || echo "Memory context monitoring not available"
}
```

### Automated Troubleshooting

```bash
#!/bin/bash
# auto_troubleshoot.sh - Automated troubleshooting for pg_hello

ISSUES_FOUND=0

check_extension_health() {
    echo "=== Extension Health Check ==="
    
    # Test basic functionality
    if ! psql -c "SELECT pg_hello('Health Check');" >/dev/null 2>&1; then
        echo "❌ pg_hello function not working"
        ((ISSUES_FOUND++))
        
        # Try to fix
        echo "Attempting to fix..."
        psql -c "DROP EXTENSION IF EXISTS pg_hello CASCADE;"
        psql -c "CREATE EXTENSION pg_hello;"
        
        if psql -c "SELECT pg_hello('Health Check');" >/dev/null 2>&1; then
            echo "✅ pg_hello function fixed"
        else
            echo "❌ Could not fix pg_hello function"
        fi
    else
        echo "✅ pg_hello function working"
    fi
}

check_performance() {
    echo "=== Performance Check ==="
    
    # Check for slow queries
    local slow_queries=$(psql -t -c "
        SELECT COUNT(*) 
        FROM pg_stat_user_functions 
        WHERE funcname IN ('pg_hello', 'now_ms', 'spi_version') 
        AND mean_time > 10;
    ")
    
    if [ "$slow_queries" -gt 0 ]; then
        echo "⚠️  $slow_queries function(s) performing slowly"
        ((ISSUES_FOUND++))
        
        # Reset statistics to get fresh data
        psql -c "SELECT pg_stat_reset();"
        echo "Statistics reset for fresh performance data"
    else
        echo "✅ Performance within acceptable limits"
    fi
}

check_security() {
    echo "=== Security Check ==="
    
    # Check file permissions
    local control_file="$(pg_config --sharedir)/extension/pg_hello.control"
    local perms=$(stat -f "%A" "$control_file" 2>/dev/null || stat -c "%a" "$control_file" 2>/dev/null)
    
    if [ "$perms" != "644" ]; then
        echo "⚠️  Incorrect permissions on control file: $perms"
        ((ISSUES_FOUND++))
        sudo chmod 644 "$control_file"
        echo "Fixed permissions on control file"
    else
        echo "✅ File permissions correct"
    fi
}

generate_report() {
    echo "=== Troubleshooting Report ==="
    echo "Timestamp: $(date)"
    echo "Issues found: $ISSUES_FOUND"
    
    if [ $ISSUES_FOUND -eq 0 ]; then
        echo "✅ All checks passed"
        exit 0
    else
        echo "⚠️  Issues found and attempted to fix"
        exit 1
    fi
}

main() {
    check_extension_health
    check_performance
    check_security
    generate_report
}

main "$@"
```

## 🔄 Maintenance Procedures

### Routine Maintenance Schedule

```bash
# /etc/cron.d/pg_hello_maintenance
# Daily maintenance at 2 AM
0 2 * * * postgres /usr/local/bin/pg_hello_daily_maintenance.sh

# Weekly maintenance on Sunday at 3 AM
0 3 * * 0 postgres /usr/local/bin/pg_hello_weekly_maintenance.sh

# Monthly maintenance on first day at 4 AM
0 4 1 * * postgres /usr/local/bin/pg_hello_monthly_maintenance.sh
```

### Daily Maintenance Script

```bash
#!/bin/bash
# pg_hello_daily_maintenance.sh

LOG_FILE="/var/log/pg_hello_maintenance.log"

log_message() {
    echo "$(date '+%Y-%m-%d %H:%M:%S') - $1" >> "$LOG_FILE"
}

# Reset statistics for fresh metrics
psql -c "SELECT pg_stat_reset();" >> "$LOG_FILE" 2>&1
log_message "Statistics reset completed"

# Check and log current performance
psql -c "SELECT * FROM pg_hello_performance_report();" >> "$LOG_FILE" 2>&1
log_message "Performance report generated"

# Health check
/usr/local/bin/health_check.sh >> "$LOG_FILE" 2>&1
log_message "Health check completed"

# Backup configuration
psql -c "
    SELECT name, setting, unit 
    FROM pg_settings 
    WHERE name LIKE 'pg_hello%'
" > "/backup/daily/pg_hello_config_$(date +%Y%m%d).txt"
log_message "Configuration backup completed"

log_message "Daily maintenance completed"
```

### Extension Updates

```bash
#!/bin/bash
# update_extension.sh - Safe extension update procedure

CURRENT_VERSION=$1
NEW_VERSION=$2

if [ -z "$CURRENT_VERSION" ] || [ -z "$NEW_VERSION" ]; then
    echo "Usage: $0 <current_version> <new_version>"
    exit 1
fi

echo "Updating pg_hello extension from $CURRENT_VERSION to $NEW_VERSION"

# Pre-update backup
echo "Creating pre-update backup..."
/usr/local/bin/backup_pg_hello.sh

# Test update in staging first
echo "Testing update in staging environment..."
psql staging -c "ALTER EXTENSION pg_hello UPDATE TO '$NEW_VERSION';"

if [ $? -eq 0 ]; then
    echo "Staging update successful"
else
    echo "Staging update failed, aborting production update"
    exit 1
fi

# Production update with rollback capability
echo "Updating production..."
DATABASES=("production" "analytics")

for db in "${DATABASES[@]}"; do
    echo "Updating database: $db"
    
    # Create transaction savepoint
    psql "$db" -c "BEGIN; SAVEPOINT pre_update;"
    
    # Perform update
    psql "$db" -c "ALTER EXTENSION pg_hello UPDATE TO '$NEW_VERSION';"
    
    if [ $? -eq 0 ]; then
        # Test functionality
        if psql "$db" -c "SELECT pg_hello('Update Test');" >/dev/null 2>&1; then
            psql "$db" -c "COMMIT;"
            echo "✅ Update successful for $db"
        else
            psql "$db" -c "ROLLBACK TO pre_update; COMMIT;"
            echo "❌ Update failed for $db - rolled back"
            exit 1
        fi
    else
        psql "$db" -c "ROLLBACK TO pre_update; COMMIT;"
        echo "❌ Update failed for $db - rolled back"
        exit 1
    fi
done

echo "✅ Extension update completed successfully"
```

This comprehensive operations guide provides everything needed to maintain a healthy, secure, and performant `pg_hello` extension in production! 🚀

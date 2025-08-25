# PostgreSQL Extension System Guide

A comprehensive guide to understanding how PostgreSQL extensions work, from installation to runtime execution.

## 📋 Table of Contents

- [What are PostgreSQL Extensions?](#what-are-postgresql-extensions)
- [Extension Architecture](#extension-architecture)
- [Extension Lifecycle](#extension-lifecycle)
- [Extension Management](#extension-management)
- [Version Management](#version-management)
- [Extension Dependencies](#extension-dependencies)
- [Security Considerations](#security-considerations)
- [Advanced Topics](#advanced-topics)
- [Debugging Extensions](#debugging-extensions)

## 🔍 What are PostgreSQL Extensions?

PostgreSQL extensions are **packaged sets of SQL objects** (functions, types, operators, etc.) that can be easily installed, managed, and removed as a unit.

### Why Extensions?

**Before Extensions** (Pre-PostgreSQL 9.1):
- Manual SQL script execution
- No dependency tracking
- Difficult to upgrade/remove
- No version management
- Security issues

**With Extensions**:
- **Atomic Installation**: All-or-nothing installation
- **Dependency Management**: Automatic dependency resolution
- **Version Control**: Clean upgrades between versions
- **Security**: Controlled installation process
- **Portability**: Works across different PostgreSQL installations

### Extension vs. Regular SQL Scripts

| Aspect | SQL Scripts | Extensions |
|--------|-------------|------------|
| Installation | Manual execution | `CREATE EXTENSION` |
| Dependencies | Manual tracking | Automatic |
| Upgrades | Manual scripts | `ALTER EXTENSION UPDATE` |
| Removal | Manual cleanup | `DROP EXTENSION CASCADE` |
| Security | Run as current user | Controlled execution |
| Portability | Path-dependent | Automatic path resolution |

## 🏗️ Extension Architecture

### Extension Components

```
Extension Package:
├── Control File (.control)      # Metadata and configuration
├── SQL Script Files (.sql)      # Object definitions  
├── Shared Library (.so/.dll)    # Compiled C code (optional)
├── Documentation Files          # Optional documentation
└── Upgrade Scripts (.sql)       # Version upgrade paths
```

### File Locations

```bash
# Extension files are installed in PostgreSQL directories:

# Control files and SQL scripts:
$SHAREDIR/extension/
├── pg_hello.control
├── pg_hello--1.0.sql
├── pg_hello--1.1.sql
└── pg_hello--1.0--1.1.sql

# Shared libraries:
$LIBDIR/
└── pg_hello.so

# Where to find these paths:
pg_config --sharedir    # /usr/share/postgresql/15
pg_config --libdir      # /usr/lib/postgresql/15/lib
```

### Control File Deep Dive

The `.control` file is the **master configuration** for the extension:

```ini
# Required fields
comment = 'Description of what the extension does'
default_version = '1.0'

# Optional but important fields
requires = 'uuid-ossp, pgcrypto'    # Comma-separated dependencies
relocatable = true                   # Can move between schemas
superuser = false                    # Requires superuser to install
trusted = true                       # Can non-superuser install in their DB
schema = 'public'                    # Fixed schema (conflicts with relocatable)
encoding = 'UTF8'                    # Required encoding
module_pathname = '$libdir/pg_hello' # Override library path
```

#### Field Explanations:

**`comment`**:
- Human-readable description
- Shown in `\dx` output
- Single line, keep concise

**`default_version`**:
- Version installed by `CREATE EXTENSION name`
- Must match a SQL file: `name--version.sql`
- Usually semantic versioning: `1.0`, `2.1.3`

**`requires`**:
- Extensions that must be installed first
- PostgreSQL checks and loads dependencies
- Comma-separated list: `'ext1, ext2, ext3'`

**`relocatable`**:
- `true`: Can use `ALTER EXTENSION SET SCHEMA`
- `false`: Fixed to one schema
- Affects how objects are created

**`superuser`**:
- `true`: Only superuser can install
- `false`: Regular users can install (if trusted)
- Security consideration for privileged operations

**`trusted`**:
- `true`: Non-superuser can install in their database
- `false`: Always requires superuser
- Newer security feature (PostgreSQL 13+)

### SQL Script Files

#### Naming Convention
```bash
# Basic version files
extensionname--version.sql          # Complete definition for version
pg_hello--1.0.sql                   # Version 1.0 definition
pg_hello--2.0.sql                   # Version 2.0 definition

# Upgrade files  
extensionname--oldver--newver.sql   # Upgrade path
pg_hello--1.0--1.1.sql             # Upgrade from 1.0 to 1.1
pg_hello--1.1--2.0.sql             # Upgrade from 1.1 to 2.0
```

#### SQL Script Content
```sql
-- pg_hello--1.0.sql

-- Function definitions
CREATE FUNCTION pg_hello(text) RETURNS text
AS '$libdir/pg_hello', 'pg_hello'
LANGUAGE C STRICT IMMUTABLE;

-- Type definitions (if any)
CREATE TYPE greeting_type AS (
    message text,
    timestamp timestamptz
);

-- Operator definitions (if any)
CREATE OPERATOR + (
    leftarg = greeting_type,
    rightarg = greeting_type,
    function = combine_greetings
);

-- Views, tables, indexes (if any)
CREATE VIEW extension_info AS
SELECT 'pg_hello' as name, '1.0' as version;
```

## 🔄 Extension Lifecycle

### 1. Development Phase

```bash
# 1. Write C code
src/pg_hello.c

# 2. Create control file
pg_hello.control

# 3. Create SQL definition
sql/pg_hello--1.0.sql

# 4. Build extension
make && make install
```

### 2. Installation Phase

```sql
-- Install extension
CREATE EXTENSION pg_hello;

-- Install specific version
CREATE EXTENSION pg_hello VERSION '1.0';

-- Install in specific schema
CREATE EXTENSION pg_hello SCHEMA my_schema;
```

**What happens during `CREATE EXTENSION`**:

1. **Validate Prerequisites**:
   - Check if dependencies are installed
   - Verify user permissions
   - Check PostgreSQL version compatibility

2. **Read Control File**:
   - Parse metadata and settings
   - Determine SQL file to execute
   - Set up execution context

3. **Execute SQL Script**:
   - Run the version-specific SQL file
   - Create all defined objects
   - Register objects as part of the extension

4. **Update System Catalogs**:
   - Record extension in `pg_extension`
   - Track all created objects in `pg_depend`
   - Set up dependency relationships

### 3. Runtime Phase

```sql
-- Use extension functions
SELECT pg_hello('World');

-- Check extension status
\dx pg_hello

-- View extension objects
SELECT * FROM pg_extension WHERE extname = 'pg_hello';
```

### 4. Upgrade Phase

```sql
-- Upgrade to newer version
ALTER EXTENSION pg_hello UPDATE TO '1.1';

-- Update to latest version
ALTER EXTENSION pg_hello UPDATE;
```

**Upgrade Process**:

1. **Find Upgrade Path**:
   - Look for `pg_hello--1.0--1.1.sql`
   - Or chain multiple upgrade files
   - Error if no path exists

2. **Execute Upgrade Script**:
   - Run the upgrade SQL commands
   - Modify existing objects
   - Add new objects as needed

3. **Update Metadata**:
   - Change version in `pg_extension`
   - Update object dependencies

### 5. Removal Phase

```sql
-- Remove extension
DROP EXTENSION pg_hello;

-- Remove with dependent objects
DROP EXTENSION pg_hello CASCADE;
```

**Removal Process**:

1. **Check Dependencies**:
   - Find objects that depend on extension
   - Error if dependencies exist (unless CASCADE)

2. **Remove Objects**:
   - Drop all extension objects in dependency order
   - Clean up system catalog entries

3. **Cleanup**:
   - Remove extension record
   - Free associated resources

## 🔧 Extension Management

### Viewing Extensions

```sql
-- List all installed extensions
\dx

-- List all available extensions
SELECT * FROM pg_available_extensions;

-- Extension details
\dx+ pg_hello

-- Extension objects
SELECT classid::regclass, objid, description 
FROM pg_description 
WHERE objoid IN (
    SELECT objid FROM pg_depend 
    WHERE refobjid = (SELECT oid FROM pg_extension WHERE extname = 'pg_hello')
);
```

### System Catalogs

#### `pg_extension`
```sql
SELECT 
    extname,           -- Extension name
    extowner,          -- Owner OID
    extnamespace,      -- Schema OID
    extrelocatable,    -- Is relocatable
    extversion,        -- Current version
    extconfig,         -- Configuration tables
    extcondition       -- WHERE conditions for config tables
FROM pg_extension 
WHERE extname = 'pg_hello';
```

#### `pg_depend`
```sql
-- Find all objects owned by extension
SELECT 
    classid::regclass as object_type,
    objid,
    objsubid,
    refclassid::regclass as referenced_type,
    refobjid
FROM pg_depend 
WHERE refobjid = (SELECT oid FROM pg_extension WHERE extname = 'pg_hello')
AND deptype = 'e';  -- 'e' = extension dependency
```

### Schema Management

```sql
-- Move extension to different schema
ALTER EXTENSION pg_hello SET SCHEMA new_schema;

-- Only works if extension is relocatable
-- Check with:
SELECT extrelocatable FROM pg_extension WHERE extname = 'pg_hello';
```

## 📈 Version Management

### Version Numbering

**Semantic Versioning** (recommended):
```
MAJOR.MINOR.PATCH
2.1.3
- MAJOR: Breaking changes
- MINOR: New features, backward compatible  
- PATCH: Bug fixes, backward compatible
```

**PostgreSQL Style**:
```
MAJOR.MINOR
15.2
- MAJOR: Major release
- MINOR: Minor updates
```

### Upgrade Paths

#### Direct Upgrades
```bash
# Direct upgrade file
pg_hello--1.0--2.0.sql
```

#### Chain Upgrades
```bash
# Multiple upgrade steps
pg_hello--1.0--1.1.sql
pg_hello--1.1--1.2.sql  
pg_hello--1.2--2.0.sql

# PostgreSQL automatically chains: 1.0 → 1.1 → 1.2 → 2.0
```

#### Upgrade Script Example
```sql
-- pg_hello--1.0--1.1.sql

-- Add new function
CREATE FUNCTION pg_hello_advanced(text, integer) RETURNS text
AS '$libdir/pg_hello', 'pg_hello_advanced'
LANGUAGE C STRICT IMMUTABLE;

-- Modify existing function (if needed)
CREATE OR REPLACE FUNCTION pg_hello(text) RETURNS text
AS '$libdir/pg_hello', 'pg_hello_v2'
LANGUAGE C STRICT IMMUTABLE;

-- Add new configuration parameter
-- (handled in C code _PG_init function)
```

### Version Control Best Practices

1. **Never modify existing version files**
   - Create new versions instead
   - Existing installations depend on exact content

2. **Provide clear upgrade paths**
   - Test all upgrade combinations
   - Document breaking changes

3. **Handle data migration**
   - Update existing data if needed
   - Provide rollback instructions

4. **Version compatibility**
   - Test with different PostgreSQL versions
   - Document version requirements

## 🔗 Extension Dependencies

### Dependency Types

#### **Hard Dependencies** (`requires`)
```ini
# In .control file
requires = 'uuid-ossp, pgcrypto'
```
- Must be installed before this extension
- Automatically loaded when extension loads
- Cannot drop dependency while extension exists

#### **Soft Dependencies** (Code-level)
```c
// In C code - check if extension exists
if (get_extension_oid("uuid-ossp", true) != InvalidOid)
{
    // uuid-ossp is available, use its functions
}
else
{
    // Provide alternative implementation
}
```

### Dependency Resolution

PostgreSQL resolves dependencies automatically:

```sql
-- Installing extension with dependencies
CREATE EXTENSION my_extension;

-- PostgreSQL automatically:
-- 1. Checks requires = 'dependency1, dependency2'
-- 2. Installs dependency1 if not present
-- 3. Installs dependency2 if not present  
-- 4. Then installs my_extension
```

### Circular Dependencies

```ini
# WRONG - creates circular dependency
# ext_a.control:
requires = 'ext_b'

# ext_b.control:  
requires = 'ext_a'
```

**Solution**: Refactor into three extensions:
- `ext_common` - shared functionality
- `ext_a` - requires `ext_common`
- `ext_b` - requires `ext_common`

## 🔒 Security Considerations

### Trust Levels

#### Trusted Extensions (PostgreSQL 13+)
```ini
# In .control file
trusted = true
superuser = false
```
- Non-superuser can install in their database
- Restricted to "safe" operations
- Cannot access filesystem, network, etc.

#### Superuser Extensions
```ini
superuser = true
trusted = false
```
- Only superuser can install
- Full system access
- Required for system-level operations

### Security Best Practices

1. **Principle of Least Privilege**
   ```sql
   -- Create functions with appropriate security
   CREATE FUNCTION safe_function(text) RETURNS text
   AS '$libdir/myext', 'safe_function'
   LANGUAGE C STRICT IMMUTABLE SECURITY DEFINER;
   ```

2. **Input Validation**
   ```c
   // Validate all inputs in C code
   if (input_value < 0 || input_value > MAX_SAFE_VALUE)
       ereport(ERROR, (errmsg("invalid input value")));
   ```

3. **Resource Limits**
   ```c
   // Prevent resource exhaustion
   if (allocation_size > MAX_ALLOCATION)
       ereport(ERROR, (errmsg("allocation too large")));
   ```

## 🔬 Advanced Topics

### Extension Configuration Tables

Some extensions need to store configuration data:

```sql
-- In extension SQL file
CREATE TABLE my_extension_config (
    key text PRIMARY KEY,
    value text
);

-- Mark as configuration table
SELECT pg_extension_config_dump('my_extension_config', '');
```

**Benefits**:
- Configuration survives `pg_dump`/`pg_restore`
- Not dropped with `DROP EXTENSION`
- Managed by extension system

### Extension Events and Hooks

```c
// In C code - register for events
static void my_extension_shmem_startup(void);

void _PG_init(void)
{
    // Register for shared memory startup
    prev_shmem_startup_hook = shmem_startup_hook;
    shmem_startup_hook = my_extension_shmem_startup;
}
```

### Background Workers

```c
#include "postmaster/bgworker.h"

void _PG_init(void)
{
    BackgroundWorker worker;
    
    memset(&worker, 0, sizeof(worker));
    worker.bgw_flags = BGWORKER_SHMEM_ACCESS | BGWORKER_BACKEND_DATABASE_CONNECTION;
    worker.bgw_start_time = BgWorkerStart_RecoveryFinished;
    strcpy(worker.bgw_library_name, "my_extension");
    strcpy(worker.bgw_function_name, "my_worker_main");
    strcpy(worker.bgw_name, "my extension worker");
    
    RegisterBackgroundWorker(&worker);
}
```

## 🐛 Debugging Extensions

### Common Issues

#### 1. Extension Won't Install
```sql
-- Check error messages
CREATE EXTENSION pg_hello;
-- ERROR: could not open extension control file...

-- Debug steps:
-- 1. Check file exists:
SELECT * FROM pg_available_extensions WHERE name = 'pg_hello';

-- 2. Check file permissions
-- 3. Check PostgreSQL logs
```

#### 2. Functions Not Found
```sql
-- ERROR: function pg_hello(text) does not exist

-- Check if extension is installed:
\dx pg_hello

-- Check function exists:
\df pg_hello
```

#### 3. Shared Library Issues
```sql
-- ERROR: could not load library "$libdir/pg_hello": 
-- dlopen: cannot load any more object

-- Debug steps:
-- 1. Check library exists:
-- ls -la $(pg_config --libdir)/pg_hello.so

-- 2. Check library dependencies:  
-- ldd $(pg_config --libdir)/pg_hello.so

-- 3. Check PostgreSQL version compatibility
```

### Debugging Tools

#### Log Analysis
```bash
# Enable detailed logging
echo "log_statement = 'all'" >> postgresql.conf
echo "log_min_messages = debug1" >> postgresql.conf

# Restart PostgreSQL and check logs
tail -f /var/log/postgresql/postgresql.log
```

#### Extension Introspection
```sql
-- Check extension objects
SELECT 
    e.extname,
    n.nspname,
    p.proname,
    p.prosrc
FROM pg_extension e
JOIN pg_depend d ON d.refobjid = e.oid
JOIN pg_proc p ON p.oid = d.objid
JOIN pg_namespace n ON n.oid = p.pronamespace
WHERE e.extname = 'pg_hello' AND d.deptype = 'e';
```

#### C-level Debugging
```bash
# Compile with debug symbols
make CFLAGS="-g -O0"

# Use gdb to debug
gdb --args postgres -D /var/lib/postgresql/data
(gdb) break pg_hello
(gdb) run
# In another terminal: psql -c "SELECT pg_hello('debug');"
```

### Testing Extensions

#### Unit Tests
```sql  
-- test/sql/basic.sql
CREATE EXTENSION pg_hello;

-- Test normal case
SELECT pg_hello('test') = 'Hello, test!' as normal_case;

-- Test edge cases
SELECT pg_hello('') = 'Hello, !' as empty_string;

-- Test configuration
SET pg_hello.repeat = 3;
SELECT pg_hello('repeat') LIKE '%Hello, repeat!%Hello, repeat!%Hello, repeat!%' as config_test;
```

#### Regression Testing
```bash
# Run automated tests
make installcheck

# Check test results
cat regression.diffs  # Shows any test failures
```

This comprehensive guide should give you a deep understanding of how PostgreSQL extensions work and how to develop, deploy, and maintain them effectively!

# File Types and Components Guide

A comprehensive guide to understanding all file types and components in PostgreSQL extension development.

## 📋 Table of Contents

- [Overview](#overview)
- [Source Files](#source-files)
- [Compiled Files](#compiled-files)
- [Configuration Files](#configuration-files)
- [SQL Files](#sql-files)
- [Test Files](#test-files)
- [Build Files](#build-files)
- [Documentation Files](#documentation-files)
- [File Relationships](#file-relationships)

## 🔍 Overview

A PostgreSQL extension consists of several different file types, each serving a specific purpose in the development, compilation, and installation process.

```
Extension Components Flow:
Source Code (.c) → Object Files (.o) → Shared Library (.so) → Installation
Configuration (.control) + SQL (.sql) → Extension Definition
```

## 📝 Source Files

### `.c` Files (C Source Code)

**Location**: `src/pg_hello.c`

**Purpose**: Contains the actual implementation of your extension functions in C language.

**What it contains**:
```c
#include "postgres.h"        // PostgreSQL core headers
#include "fmgr.h"           // Function manager interface

// Function implementations
Datum pg_hello(PG_FUNCTION_ARGS) {
    // Your C code here
}
```

**Key sections**:
1. **Headers**: Include PostgreSQL API definitions
2. **Module Magic**: `PG_MODULE_MAGIC` for version compatibility
3. **Function Declarations**: `PG_FUNCTION_INFO_V1(function_name)`
4. **Initialization**: `_PG_init()` for extension startup
5. **Function Implementations**: Actual C functions

**Why C?**:
- **Performance**: Native code execution
- **PostgreSQL Integration**: Direct access to internal APIs
- **Memory Management**: Control over memory allocation
- **System Access**: Can call system functions and libraries

### `.h` Files (Header Files)

**Purpose**: Contain function declarations, constants, and type definitions.

**Example content**:
```c
#ifndef PG_HELLO_H
#define PG_HELLO_H

// Function declarations
extern Datum pg_hello(PG_FUNCTION_ARGS);
extern Datum now_ms(PG_FUNCTION_ARGS);

// Constants
#define MAX_GREETING_LENGTH 1024

#endif /* PG_HELLO_H */
```

**When needed**:
- Multiple source files
- Shared constants or types
- External library interfaces

## 🔧 Compiled Files

### `.o` Files (Object Files)

**Example**: `pg_hello.o`

**What it is**:
- **Compiled Code**: Machine code compiled from `.c` files
- **Not Executable**: Contains compiled functions but not a complete program
- **Platform Specific**: Different for x86, ARM, etc.
- **Intermediate Step**: Used to create the final shared library

**Creation Process**:
```bash
# This command creates the .o file:
clang -c -o pg_hello.o src/pg_hello.c
```

**What's inside**:
- **Machine Code**: Binary instructions for the CPU
- **Symbol Table**: Names of functions and variables
- **Relocation Info**: How to connect to other code
- **Debug Info**: If compiled with `-g` flag

**File Format**: 
- **Linux**: ELF (Executable and Linkable Format)
- **macOS**: Mach-O (Mach Object)
- **Windows**: COFF (Common Object File Format)

**Why needed**:
1. **Incremental Compilation**: Only recompile changed files
2. **Modularity**: Combine multiple `.o` files
3. **Linking**: Connect to libraries and other objects
4. **Debugging**: Contains symbol information

### `.so` Files (Shared Object/Library)

**Example**: `pg_hello.so`

**What it is**:
- **Shared Library**: Complete loadable module
- **Dynamic Loading**: Loaded at runtime by PostgreSQL
- **Contains**: All functions from the extension
- **Platform Specific**: `.so` on Linux/Unix, `.dylib` on macOS, `.dll` on Windows

**Creation Process**:
```bash
# This creates the .so file from .o files:
clang -shared -o pg_hello.so pg_hello.o
```

**What's inside**:
- **Executable Code**: Ready-to-run machine instructions
- **Export Table**: Functions available to PostgreSQL
- **Dependencies**: References to other libraries
- **Metadata**: Version info, symbols, etc.

**How PostgreSQL uses it**:
1. **Loading**: `LOAD 'pg_hello'` loads the .so file
2. **Function Calls**: PostgreSQL calls functions in the library
3. **Memory**: Code stays in memory while PostgreSQL runs
4. **Unloading**: Removed when PostgreSQL shuts down

**Advantages of Shared Libraries**:
- **Memory Efficiency**: One copy shared by all processes
- **Updates**: Can update library without recompiling PostgreSQL
- **Modularity**: Add/remove functionality dynamically

## ⚙️ Configuration Files

### `.control` Files (Extension Control)

**Example**: `pg_hello.control`

**Purpose**: Tells PostgreSQL how to install and manage the extension.

**Content breakdown**:
```ini
# Extension metadata
comment = 'Tiny sample extension (C functions, GUC, SPI demo)'
default_version = '1.0'
relocatable = true
requires = ''
```

**Field explanations**:

#### `comment`
- **Purpose**: Human-readable description
- **Usage**: Shown in `\dx` command
- **Example**: `'My custom extension for text processing'`

#### `default_version`
- **Purpose**: Version to install by default
- **Format**: Usually semantic versioning (1.0, 1.2.3)
- **Usage**: `CREATE EXTENSION pg_hello` uses this version

#### `relocatable`
- **Purpose**: Can the extension be moved between schemas?
- **Values**: `true` or `false`
- **Impact**: 
  - `true`: Can use `ALTER EXTENSION ... SET SCHEMA`
  - `false`: Fixed to specific schema

#### `requires`
- **Purpose**: List of required extensions
- **Format**: Comma-separated list
- **Example**: `'uuid-ossp, pgcrypto'`

#### Other possible fields:
```ini
# Advanced options
schema = 'public'           # Fixed schema (conflicts with relocatable=true)
superuser = true           # Requires superuser to install
trusted = false            # Can non-superuser install?
module_pathname = '$libdir/pg_hello'  # Custom library path
```

**File location after installation**:
```bash
# Usually installed to:
/usr/share/postgresql/15/extension/pg_hello.control
```

### `.sql` Files (SQL Definitions)

**Example**: `sql/pg_hello--1.0.sql`

**Purpose**: Defines the SQL interface for your extension.

**Naming Convention**: `extensionname--version.sql`

**Content structure**:
```sql
-- Function definitions
CREATE FUNCTION pg_hello(text) RETURNS text
AS '$libdir/pg_hello', 'pg_hello'
LANGUAGE C STRICT IMMUTABLE;

-- Type definitions (if any)
CREATE TYPE my_type AS (
    field1 integer,
    field2 text
);

-- Operators (if any)
CREATE OPERATOR + (
    leftarg = my_type,
    rightarg = my_type,
    function = my_type_add
);
```

**Key components**:

#### Function Definitions
```sql
CREATE FUNCTION function_name(parameters) RETURNS return_type
AS 'library_path', 'c_function_name'
LANGUAGE C [STRICT] [IMMUTABLE|STABLE|VOLATILE];
```

- **library_path**: Usually `$libdir/extension_name`
- **c_function_name**: Exact name of C function
- **STRICT**: NULL input → NULL output (no function call)
- **IMMUTABLE**: Same input always gives same output
- **STABLE**: Same input gives same output within transaction
- **VOLATILE**: Output can change (default)

#### Library Path Options
```sql
-- Standard location
AS '$libdir/pg_hello'

-- Absolute path
AS '/usr/local/lib/pg_hello'

-- Relative to libdir
AS '$libdir/extensions/pg_hello'
```

**Version Management**:
```bash
# For version 1.0
sql/pg_hello--1.0.sql

# For version 1.1
sql/pg_hello--1.1.sql

# Upgrade from 1.0 to 1.1
sql/pg_hello--1.0--1.1.sql
```

## 🧪 Test Files

### Test SQL Files

**Location**: `test/sql/basic.sql`

**Purpose**: Contains SQL commands to test your extension.

**Example content**:
```sql
-- Load the extension
CREATE EXTENSION pg_hello;

-- Test basic function
SELECT pg_hello('Test') as result;

-- Test configuration
SET pg_hello.repeat = 3;
SELECT pg_hello('Multiple') as repeated;

-- Test edge cases
SELECT pg_hello('') as empty_string;
SELECT pg_hello(NULL) as null_input;
```

**Best practices**:
- **Comprehensive**: Test all functions and edge cases
- **Deterministic**: Results should be predictable
- **Clean**: Don't leave test data behind
- **Documented**: Comments explaining what's being tested

### Expected Output Files

**Location**: `test/expected/basic.out`

**Purpose**: Contains the expected output for test comparisons.

**Format**: Exact PostgreSQL output including:
```
CREATE EXTENSION
   result    
-------------
 Hello, Test!
(1 row)

SET
      repeated       
---------------------
 Hello, Multiple! Hello, Multiple! Hello, Multiple!
(1 row)
```

**Generation**:
```bash
# Run test and capture output
psql -d test_db -f test/sql/basic.sql > test/expected/basic.out

# Clean up the output file manually
# Remove timestamps, connection info, etc.
```

## 🔨 Build Files

### `Makefile`

**Purpose**: Controls the build process.

**Key sections**:
```makefile
# What to build
EXTENSION = pg_hello
MODULES = pg_hello
DATA = sql/pg_hello--1.0.sql

# How to build custom files
pg_hello.o: src/pg_hello.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

# PostgreSQL integration
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
```

**See**: [MAKEFILE.md](MAKEFILE.md) for detailed explanation.

### Temporary Build Files

During compilation, you might see:

#### `.deps/` Directory
- **Purpose**: Dependency tracking
- **Contents**: `.Po` files with dependency information
- **Generated**: Automatically by make
- **Ignored**: Usually in `.gitignore`

#### Core Files
- **Format**: `core` or `core.12345`
- **Purpose**: Memory dumps from crashes
- **Action**: Delete and investigate the crash

## 📚 Documentation Files

### `README.md`
**Purpose**: Main documentation and usage guide.

### `docs/` Directory
**Purpose**: Detailed technical documentation.

**Typical contents**:
- `INSTALL.md` - Installation instructions
- `API.md` - Function reference
- `CHANGELOG.md` - Version history
- `CONTRIBUTING.md` - Development guidelines

## 🔗 File Relationships

### Compilation Flow
```
src/pg_hello.c  →  [compiler]  →  pg_hello.o  →  [linker]  →  pg_hello.so
     ↑                                                           ↓
 (edit code)                                             (install to PostgreSQL)
```

### Installation Flow
```
pg_hello.control     →  $sharedir/extension/
sql/pg_hello--1.0.sql  →  $sharedir/extension/
pg_hello.so         →  $libdir/
```

### Runtime Flow
```
CREATE EXTENSION pg_hello;
    ↓
1. Read pg_hello.control
2. Execute sql/pg_hello--1.0.sql
3. Register functions pointing to pg_hello.so
    ↓
SELECT pg_hello('World');
    ↓
4. Load pg_hello.so (if not already loaded)
5. Call pg_hello() function in the shared library
```

### Development Cycle
```
1. Edit src/pg_hello.c
2. make clean && make          # Creates .o and .so
3. make install                # Copies files to PostgreSQL
4. psql -c "DROP EXTENSION IF EXISTS pg_hello; CREATE EXTENSION pg_hello;"
5. Test functions
6. make installcheck           # Run automated tests
7. Repeat from step 1
```

## 🛠️ File Management Commands

### Viewing File Types
```bash
# Check file types
file pg_hello.o     # pg_hello.o: Mach-O 64-bit object arm64
file pg_hello.so    # pg_hello.so: Mach-O 64-bit dynamically linked shared library arm64

# List symbols in object files
nm pg_hello.o       # Show symbols
objdump -t pg_hello.o   # Detailed symbol table

# Check shared library dependencies
ldd pg_hello.so     # Linux
otool -L pg_hello.so    # macOS
```

### Finding Installed Files
```bash
# Find where PostgreSQL installs files
pg_config --sharedir     # /usr/share/postgresql/15
pg_config --libdir       # /usr/lib/postgresql/15/lib

# Find your extension files
find $(pg_config --sharedir) -name "*pg_hello*"
find $(pg_config --libdir) -name "*pg_hello*"
```

### Cleaning Up
```bash
# Remove built files
make clean

# Remove installed files
make uninstall

# Remove from database
psql -c "DROP EXTENSION pg_hello;"
```

## 🐛 Common Issues

### File Permission Problems
```bash
# .so file not executable
chmod +x pg_hello.so

# Can't install (need sudo)
sudo make install
```

### File Not Found Errors
```bash
# Check if files exist
ls -la pg_hello.so
ls -la $(pg_config --libdir)/pg_hello.so

# Check PostgreSQL can find it
psql -c "SELECT pg_loaded_libraries();"
```

### Version Mismatches
```bash
# Check PostgreSQL version
pg_config --version

# Check extension version
psql -c "\dx pg_hello"

# Rebuild for correct version
make clean && make && make install
```

## 📋 File Checklist

Before releasing an extension, ensure you have:

- [ ] **Source Code**: `src/*.c` files with proper headers
- [ ] **Control File**: `extension.control` with correct metadata
- [ ] **SQL Definitions**: `sql/extension--version.sql` files
- [ ] **Makefile**: Proper build configuration
- [ ] **Tests**: `test/sql/*.sql` and `test/expected/*.out`
- [ ] **Documentation**: `README.md` and other docs
- [ ] **License**: License file if distributing
- [ ] **Gitignore**: Excludes `*.o`, `*.so`, build artifacts

This comprehensive understanding of file types will help you navigate PostgreSQL extension development with confidence!

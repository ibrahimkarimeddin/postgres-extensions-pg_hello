# Build Process and Installation Guide

A comprehensive guide to understanding the complete build process, from source code to installed PostgreSQL extension.

## 📋 Table of Contents

- [Overview](#overview)
- [Prerequisites](#prerequisites)
- [Build Environment Setup](#build-environment-setup)
- [Step-by-Step Build Process](#step-by-step-build-process)
- [Installation Process](#installation-process)
- [Testing and Validation](#testing-and-validation)
- [Cross-Platform Considerations](#cross-platform-considerations)
- [Troubleshooting](#troubleshooting)
- [Advanced Build Topics](#advanced-build-topics)

## 🔍 Overview

The PostgreSQL extension build process transforms your source code into a loadable module that PostgreSQL can use at runtime.

### Build Flow Overview

```
Source Files        Build Process           Installation         Runtime
     ↓                     ↓                      ↓                ↓
┌──────────┐        ┌─────────────┐        ┌─────────────┐   ┌─────────────┐
│ .c files │   →    │ Compilation │   →    │ File Copy   │ → │ Extension   │
│ .control │        │             │        │             │   │ Loading     │
│ .sql     │        │ Linking     │        │ Registration│   │             │
│ Makefile │        │             │        │             │   │ Function    │
└──────────┘        └─────────────┘        └─────────────┘   │ Execution   │
                                                             └─────────────┘
```

### What Gets Built

1. **Object Files** (`.o`) - Compiled machine code
2. **Shared Library** (`.so`/`.dll`/`.dylib`) - Loadable module
3. **Extension Files** - Control and SQL files ready for installation

## 📋 Prerequisites

### System Requirements

#### Linux (Ubuntu/Debian)
```bash
# Essential build tools
sudo apt-get install build-essential

# PostgreSQL development headers
sudo apt-get install postgresql-server-dev-all

# Additional tools
sudo apt-get install pkg-config git
```

#### Linux (CentOS/RHEL/Fedora)
```bash
# Essential build tools
sudo yum groupinstall "Development Tools"
# or for newer versions:
sudo dnf groupinstall "Development Tools"

# PostgreSQL development headers
sudo yum install postgresql-devel
# or:
sudo dnf install postgresql-devel
```

#### macOS
```bash
# Install Xcode command line tools
xcode-select --install

# Install PostgreSQL via Homebrew
brew install postgresql@15

# Or install from PostgreSQL.org installer
# Then ensure pg_config is in PATH
export PATH=/usr/local/pgsql/bin:$PATH
```

#### Windows
```powershell
# Install Visual Studio Build Tools
# Install PostgreSQL from postgresql.org
# Ensure pg_config.exe is in PATH

# Add to PATH (example):
$env:PATH += ";C:\Program Files\PostgreSQL\15\bin"
```

### Verification

```bash
# Check compiler
gcc --version        # Linux
clang --version      # macOS

# Check PostgreSQL development environment
pg_config --version
pg_config --includedir-server
pg_config --libdir
pg_config --pgxs

# Check make
make --version
```

## ⚙️ Build Environment Setup

### Environment Variables

```bash
# Essential PostgreSQL paths
export PG_CONFIG=$(which pg_config)
export PGXS=$(pg_config --pgxs)

# Additional paths (if needed)
export PG_INCLUDE=$(pg_config --includedir-server)
export PG_LIB=$(pg_config --libdir)

# Compiler options (optional)
export CC=clang                    # Use specific compiler
export CFLAGS="-g -O2 -Wall"      # Custom compiler flags
```

### Development Directory Structure

```
project/
├── src/                    # Source code
│   ├── pg_hello.c         # Main implementation
│   └── helper.c           # Additional modules (if any)
├── sql/                   # SQL definitions
│   ├── pg_hello--1.0.sql  # Version 1.0
│   └── pg_hello--1.1.sql  # Version 1.1 (if exists)
├── test/                  # Tests
│   ├── sql/
│   │   └── basic.sql      # Test SQL
│   └── expected/
│       └── basic.out      # Expected output
├── docs/                  # Documentation
├── Makefile              # Build configuration
├── pg_hello.control      # Extension metadata
└── README.md             # Project documentation
```

## 🔨 Step-by-Step Build Process

### Phase 1: Preprocessing

```bash
# What happens: C preprocessor processes #include and #define
# Command (simplified):
cpp -I$(pg_config --includedir-server) src/pg_hello.c > pg_hello.i
```

**Purpose**:
- Include header files
- Process macros and defines
- Handle conditional compilation

**Output**: Preprocessed source code (`.i` files)

### Phase 2: Compilation

```bash
# What happens: C compiler converts preprocessed code to object code
# Actual command:
$(CC) $(CFLAGS) $(CPPFLAGS) -c -o pg_hello.o src/pg_hello.c
```

**Detailed command breakdown**:
```bash
clang \
  -Wall -Wmissing-prototypes -Wpointer-arith \           # Warning flags
  -Wdeclaration-after-statement -Werror=vla \           # More warnings
  -Wendif-labels -Wmissing-format-attribute \           # Even more warnings
  -Wcast-function-type -Wformat-security \              # Security warnings
  -fno-strict-aliasing -fwrapv \                        # Safety flags
  -fexcess-precision=standard \                         # Precision control
  -O2 \                                                 # Optimization level
  -I. -I./ \                                           # Local includes
  -I/opt/homebrew/opt/postgresql@15/include/postgresql/server \ # PG headers
  -I/opt/homebrew/opt/postgresql@15/include/postgresql/internal \ # PG internal
  -isysroot /Library/Developer/CommandLineTools/SDKs/MacOSX15.sdk \ # macOS SDK
  -c \                                                  # Compile only (no link)
  -o pg_hello.o \                                      # Output file
  src/pg_hello.c                                       # Input file
```

**What each flag means**:
- **`-Wall`**: Enable common warnings
- **`-Wmissing-prototypes`**: Warn about missing function declarations
- **`-Wpointer-arith`**: Warn about pointer arithmetic
- **`-Wdeclaration-after-statement`**: Enforce C90 style (declarations first)
- **`-O2`**: Optimization level 2 (good performance, reasonable compile time)
- **`-I<path>`**: Add include directory
- **`-c`**: Compile to object file only (don't link)
- **`-o`**: Specify output file name

**Output**: Object file (`pg_hello.o`)

### Phase 3: Linking

```bash
# What happens: Linker combines object files into shared library
# Command:
$(CC) -shared $(LDFLAGS) -o pg_hello.so pg_hello.o $(LIBS)
```

**Detailed linking**:
```bash
clang \
  -shared \                                            # Create shared library
  -o pg_hello.so \                                    # Output file
  pg_hello.o                                          # Input object file
```

**What linking does**:
- Combines multiple object files (if any)
- Resolves external symbols
- Creates loadable shared library
- Sets up dynamic loading information

**Output**: Shared library (`pg_hello.so`, `pg_hello.dll`, or `pg_hello.dylib`)

### Phase 4: Validation

```bash
# Check the created shared library
file pg_hello.so
nm pg_hello.so          # List symbols
ldd pg_hello.so         # Check dependencies (Linux)
otool -L pg_hello.so    # Check dependencies (macOS)
```

### Complete Build Command

```bash
# Run the entire build process
make clean    # Remove old files
make          # Build everything
```

## 📦 Installation Process

### Phase 1: File Installation

```bash
# Install command
make install
```

**What gets installed**:

1. **Control File**:
   ```bash
   # From: ./pg_hello.control
   # To: $(pg_config --sharedir)/extension/pg_hello.control
   install -m 644 pg_hello.control /usr/share/postgresql/15/extension/
   ```

2. **SQL Files**:
   ```bash
   # From: sql/pg_hello--1.0.sql
   # To: $(pg_config --sharedir)/extension/pg_hello--1.0.sql
   install -m 644 sql/pg_hello--1.0.sql /usr/share/postgresql/15/extension/
   ```

3. **Shared Library**:
   ```bash
   # From: ./pg_hello.so
   # To: $(pg_config --libdir)/pg_hello.so
   install -m 755 pg_hello.so /usr/lib/postgresql/15/lib/
   ```

### Phase 2: PostgreSQL Registration

The extension files are now available to PostgreSQL, but not yet active in any database.

### Phase 3: Database Extension Creation

```sql
-- This command activates the extension in the current database
CREATE EXTENSION pg_hello;
```

**What happens during CREATE EXTENSION**:

1. **Read Control File**:
   - Parse `pg_hello.control`
   - Check dependencies
   - Validate permissions

2. **Execute SQL Script**:
   - Run `sql/pg_hello--1.0.sql`
   - Create functions, types, operators
   - Register with PostgreSQL catalogs

3. **Update System Catalogs**:
   - Add entry to `pg_extension`
   - Create dependency entries in `pg_depend`
   - Update object ownership

## 🧪 Testing and Validation

### Build Testing

```bash
# Test compilation without installation
make clean && make

# Check for warnings
make 2>&1 | grep -i warning

# Validate shared library
file pg_hello.so
```

### Installation Testing

```bash
# Install extension
make install

# Verify files are in place
ls -la $(pg_config --sharedir)/extension/pg_hello*
ls -la $(pg_config --libdir)/pg_hello*
```

### Functional Testing

```sql
-- Test extension creation
CREATE EXTENSION pg_hello;

-- Test basic functionality
SELECT pg_hello('Test');

-- Test configuration
SET pg_hello.repeat = 2;
SELECT pg_hello('Config');

-- Test all functions
SELECT now_ms() > 0;
SELECT spi_version() LIKE 'PostgreSQL%';
```

### Regression Testing

```bash
# Run automated tests
make installcheck

# Check results
echo $?          # Should be 0 for success
cat regression.diffs  # Shows any differences
```

## 🌐 Cross-Platform Considerations

### Platform-Specific Files

| Platform | Shared Library | Compiler | Linker Flags |
|----------|----------------|----------|--------------|
| Linux | `.so` | `gcc` | `-shared -fPIC` |
| macOS | `.dylib` | `clang` | `-dynamiclib` |
| Windows | `.dll` | `cl.exe` | `/LD` |

### Compiler Differences

#### GCC (Linux)
```bash
# Typical flags
-fPIC              # Position Independent Code
-shared            # Create shared library
-Wl,-soname,name   # Set shared object name
```

#### Clang (macOS)
```bash
# Typical flags
-dynamiclib        # Create dynamic library
-undefined dynamic_lookup  # Allow undefined symbols
-install_name @rpath/name  # Set install name
```

#### MSVC (Windows)
```cmd
REM Typical flags
/LD               REM Create DLL
/MD               REM Use dynamic runtime
```

### Path Handling

```bash
# Unix-style paths
/usr/share/postgresql/15/extension/

# Windows-style paths
C:\Program Files\PostgreSQL\15\share\extension\
```

### File Permissions

```bash
# Unix permissions
chmod 644 *.control *.sql    # Read for all, write for owner
chmod 755 *.so              # Execute for all, write for owner

# Windows permissions
# Usually handled by installer
```

## 🐛 Troubleshooting

### Common Build Errors

#### 1. `pg_config not found`

**Error**:
```
make: pg_config: command not found
```

**Solutions**:
```bash
# Add PostgreSQL to PATH
export PATH=/usr/pgsql-15/bin:$PATH

# Or specify explicitly in Makefile
PG_CONFIG = /usr/pgsql-15/bin/pg_config
```

#### 2. Missing Headers

**Error**:
```
fatal error: postgres.h: No such file or directory
```

**Solutions**:
```bash
# Install development packages
# Ubuntu/Debian:
sudo apt-get install postgresql-server-dev-all

# CentOS/RHEL:
sudo yum install postgresql-devel
```

#### 3. Compiler Warnings

**Error**:
```
warning: mixing declarations and code is incompatible with standards before C99
```

**Solution**:
```c
// Move all variable declarations to function start
Datum my_function(PG_FUNCTION_ARGS)
{
    // All declarations first
    text *input;
    char *cstring;
    int result;
    
    // Then code
    input = PG_GETARG_TEXT_PP(0);
    // ...
}
```

#### 4. Linking Errors

**Error**:
```
undefined reference to 'some_function'
```

**Solutions**:
```makefile
# Add missing libraries
SHLIB_LINK = -lm -lpthread

# Add library paths
LDFLAGS += -L/usr/local/lib
```

#### 5. Permission Errors

**Error**:
```
Permission denied: cannot create /usr/lib/postgresql/15/lib/pg_hello.so
```

**Solution**:
```bash
# Use sudo for installation
sudo make install

# Or install to custom location
make install DESTDIR=/tmp/pg_hello-install
```

### Installation Troubleshooting

#### 1. Extension Not Available

**Error**:
```
ERROR: extension "pg_hello" is not available
```

**Debug**:
```sql
-- Check available extensions
SELECT * FROM pg_available_extensions WHERE name = 'pg_hello';

-- Check file exists
\! ls -la $(pg_config --sharedir)/extension/pg_hello*
```

#### 2. Library Load Error

**Error**:
```
ERROR: could not load library "$libdir/pg_hello": dlopen: cannot load any more object
```

**Debug**:
```bash
# Check library exists
ls -la $(pg_config --libdir)/pg_hello.so

# Check dependencies
ldd $(pg_config --libdir)/pg_hello.so    # Linux
otool -L $(pg_config --libdir)/pg_hello.so  # macOS

# Check PostgreSQL logs
tail -f /var/log/postgresql/postgresql.log
```

### Debug Build

```bash
# Build with debug information
make CFLAGS="-g -O0 -DDEBUG"

# Use with debugger
gdb --args postgres -D /var/lib/postgresql/data
(gdb) break pg_hello
(gdb) run
```

## 🔬 Advanced Build Topics

### Custom Build Variables

```makefile
# Override default settings
EXTENSION = my_extension
MODULES = my_extension helper_module
DATA = sql/my_extension--1.0.sql sql/my_extension--1.1.sql
DOCS = README.md docs/manual.html

# Custom compiler flags
PG_CPPFLAGS = -DDEBUG -I/usr/local/include
SHLIB_LINK = -lm -lpthread -lssl

# Custom source locations
my_extension.o: src/main.c src/helper.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c src/main.c src/helper.c
```

### Conditional Compilation

```c
// Version-specific code
#if PG_VERSION_NUM >= 150000
    // PostgreSQL 15+ code
    result = new_function(input);
#else
    // Older PostgreSQL code
    result = old_function(input);
#endif
```

### Multiple Architecture Support

```bash
# Build for multiple architectures (macOS)
make CFLAGS="-arch x86_64 -arch arm64"

# Separate builds for different PostgreSQL versions
make clean && PG_CONFIG=/usr/pgsql-14/bin/pg_config make
make clean && PG_CONFIG=/usr/pgsql-15/bin/pg_config make
```

### Packaging

```bash
# Create distribution package
make dist

# Create RPM package (CentOS/RHEL)
rpmbuild -ba postgresql-pg_hello.spec

# Create DEB package (Ubuntu/Debian)
dpkg-buildpackage -b

# Create installer (Windows)
# Use NSIS or similar installer creator
```

This comprehensive guide should help you understand every aspect of building and installing PostgreSQL extensions!

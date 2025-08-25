# Makefile Documentation

A comprehensive guide to understanding the Makefile used in PostgreSQL extension development.

## 📋 Table of Contents

- [What is Make?](#what-is-make)
- [Understanding Our Makefile](#understanding-our-makefile)
- [PGXS System](#pgxs-system)
- [Make Commands](#make-commands)
- [Variables Explained](#variables-explained)
- [Build Process](#build-process)
- [Customization](#customization)
- [Troubleshooting](#troubleshooting)

## 🔧 What is Make?

**Make** is a build automation tool that manages the compilation and linking of programs. It:

- Reads instructions from a file called `Makefile`
- Determines which files need to be rebuilt
- Executes commands in the correct order
- Only rebuilds what's necessary (dependency tracking)

### Why Use Make?

1. **Automation**: No need to remember complex compiler commands
2. **Efficiency**: Only recompiles changed files
3. **Dependencies**: Ensures prerequisites are built first
4. **Portability**: Works across different Unix-like systems
5. **Integration**: Standard tool in software development

## 📁 Understanding Our Makefile

Let's break down our PostgreSQL extension Makefile line by line:

```makefile
# Our complete Makefile
EXTENSION = pg_hello
MODULES = pg_hello
DATA = sql/pg_hello--1.0.sql
PGFILEDESC = "pg_hello - tiny sample extension (C functions, GUC)"

# Specify the source file location explicitly
pg_hello.o: src/pg_hello.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)

REGRESS = basic
```

### Line-by-Line Explanation

#### 1. Extension Definition
```makefile
EXTENSION = pg_hello
```
- **Purpose**: Defines the name of our extension
- **Effect**: Creates `pg_hello.so` shared library
- **Must Match**: The `.control` file name (`pg_hello.control`)

#### 2. Module Definition
```makefile
MODULES = pg_hello
```
- **Purpose**: Lists the modules (shared libraries) to build
- **Effect**: Tells PGXS to compile `pg_hello.c` into `pg_hello.so`
- **Multiple Modules**: Can list multiple: `MODULES = module1 module2`

#### 3. SQL Data Files
```makefile
DATA = sql/pg_hello--1.0.sql
```
- **Purpose**: Lists SQL files to install with the extension
- **Location**: Installed to PostgreSQL's extension directory
- **Pattern**: Usually follows `extensionname--version.sql` format

#### 4. Description
```makefile
PGFILEDESC = "pg_hello - tiny sample extension (C functions, GUC)"
```
- **Purpose**: Human-readable description of the extension
- **Usage**: Appears in package metadata and documentation
- **Optional**: But recommended for clarity

#### 5. Custom Build Rule
```makefile
pg_hello.o: src/pg_hello.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<
```
- **Purpose**: Custom rule to compile our C source from `src/` directory
- **Breakdown**:
  - `pg_hello.o:` - Target file (object file)
  - `src/pg_hello.c` - Dependency (source file)
  - `$(CC)` - Compiler variable (usually gcc or clang)
  - `$(CFLAGS)` - Compiler flags for optimization, warnings
  - `$(CPPFLAGS)` - Preprocessor flags for include paths
  - `-c` - Compile only, don't link
  - `-o $@` - Output to target name (`pg_hello.o`)
  - `$<` - First dependency (`src/pg_hello.c`)

#### 6. PostgreSQL Configuration
```makefile
PG_CONFIG = pg_config
```
- **Purpose**: Specifies the PostgreSQL configuration tool
- **Function**: `pg_config` provides PostgreSQL installation paths and settings
- **Customizable**: Can point to specific PostgreSQL version

#### 7. PGXS Integration
```makefile
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
```
- **Purpose**: Includes PostgreSQL Extension Building Infrastructure
- **Breakdown**:
  - `$(shell ...)` - Executes command and captures output
  - `$(PG_CONFIG) --pgxs` - Gets path to PGXS makefile
  - `include $(PGXS)` - Includes all PostgreSQL build rules

#### 8. Regression Tests
```makefile
REGRESS = basic
```
- **Purpose**: Defines test files to run with `make installcheck`
- **Files**: Looks for `test/sql/basic.sql` and `test/expected/basic.out`
- **Multiple Tests**: Can list multiple: `REGRESS = basic advanced`

## 🔗 PGXS System

**PGXS** (PostgreSQL Extension Building Infrastructure) is PostgreSQL's standard system for building extensions.

### What PGXS Provides

1. **Standard Variables**: `CC`, `CFLAGS`, `LDFLAGS`, etc.
2. **Build Rules**: How to compile `.c` to `.o` to `.so`
3. **Installation Rules**: Where to put files
4. **Test Framework**: Regression testing infrastructure
5. **Cross-Platform Support**: Works on Linux, macOS, Windows

### PGXS Variables You Can Use

```makefile
# Compilation
MODULES = list_of_modules
EXTENSION = extension_name
DATA = list_of_sql_files
DOCS = list_of_documentation_files

# Advanced
SHLIB_LINK = additional_libraries_to_link
PG_CPPFLAGS = additional_preprocessor_flags
EXTRA_CLEAN = additional_files_to_clean

# Testing
REGRESS = list_of_test_names
REGRESS_OPTS = options_for_pg_regress
```

### PGXS Rules You Get

```bash
# Build rules
make                # Compile the extension
make clean          # Remove built files
make install        # Install to PostgreSQL
make installcheck   # Run regression tests
make uninstall      # Remove from PostgreSQL

# Advanced rules
make submake        # Handle subdirectories
make dist           # Create distribution package
```

## 🚀 Make Commands

### Basic Commands

#### `make` (or `make all`)
```bash
make
```
- **Purpose**: Builds the extension
- **Process**:
  1. Compiles `src/pg_hello.c` to `pg_hello.o`
  2. Links `pg_hello.o` to `pg_hello.so`
  3. Prepares for installation

#### `make clean`
```bash
make clean
```
- **Purpose**: Removes all built files
- **Removes**:
  - Object files (`.o`)
  - Shared libraries (`.so`)
  - Temporary files
- **When to Use**: Before fresh rebuild, before committing to git

#### `make install`
```bash
make install
# or with sudo for system-wide installation
sudo make install
```
- **Purpose**: Installs extension files to PostgreSQL
- **Installs**:
  - `pg_hello.so` → `$libdir`
  - `pg_hello.control` → `$sharedir/extension/`
  - `pg_hello--1.0.sql` → `$sharedir/extension/`
- **Requires**: Write permissions to PostgreSQL directories

#### `make installcheck`
```bash
make installcheck
```
- **Purpose**: Runs regression tests
- **Process**:
  1. Creates test database
  2. Runs `test/sql/*.sql` files
  3. Compares output with `test/expected/*.out`
  4. Reports differences

### Advanced Commands

#### `make uninstall`
```bash
sudo make uninstall
```
- **Purpose**: Removes extension from PostgreSQL
- **Removes**: All installed files
- **Note**: Doesn't affect databases using the extension

#### `make dist`
```bash
make dist
```
- **Purpose**: Creates distribution package
- **Output**: Tarball with source files
- **Usage**: For sharing or packaging

## 📊 Variables Explained

### Automatic Variables

These are set by Make automatically:

- `$@` - Target name (e.g., `pg_hello.o`)
- `$<` - First dependency (e.g., `src/pg_hello.c`)
- `$^` - All dependencies
- `$?` - Dependencies newer than target

### PostgreSQL Variables (from PGXS)

```bash
# Paths
$(bindir)       # PostgreSQL bin directory
$(libdir)       # PostgreSQL lib directory  
$(sharedir)     # PostgreSQL share directory
$(includedir)   # PostgreSQL include directory

# Tools
$(CC)           # C compiler
$(LD)           # Linker
$(PG_CONFIG)    # pg_config tool

# Flags
$(CFLAGS)       # C compiler flags
$(CPPFLAGS)     # C preprocessor flags
$(LDFLAGS)      # Linker flags
```

### Example: Finding PostgreSQL Paths

```bash
# See what pg_config provides
pg_config --bindir      # /usr/pgsql-15/bin
pg_config --libdir      # /usr/pgsql-15/lib
pg_config --sharedir    # /usr/pgsql-15/share
pg_config --includedir  # /usr/pgsql-15/include
pg_config --pgxs        # /usr/pgsql-15/lib/pgxs/src/makefiles/pgxs.mk
```

## 🔄 Build Process

### Step-by-Step Build Process

1. **Parse Makefile**
   - Read variables and rules
   - Include PGXS system

2. **Check Dependencies**
   - Compare timestamps
   - Determine what needs rebuilding

3. **Compile Source**
   ```bash
   clang -Wall -Wmissing-prototypes ... -c -o pg_hello.o src/pg_hello.c
   ```

4. **Link Shared Library**
   ```bash
   clang -shared -o pg_hello.so pg_hello.o
   ```

5. **Prepare Installation Files**
   - Verify control file
   - Check SQL files

### Dependency Chain

```
pg_hello.so
    ↑
pg_hello.o
    ↑
src/pg_hello.c
```

- If `src/pg_hello.c` changes → rebuild `pg_hello.o`
- If `pg_hello.o` changes → rebuild `pg_hello.so`
- If nothing changes → no rebuild needed

## 🛠️ Customization

### Adding Custom Compiler Flags

```makefile
# Add debugging information
PG_CPPFLAGS = -DDEBUG -g

# Add additional include paths
PG_CPPFLAGS = -I/usr/local/include

# Link additional libraries
SHLIB_LINK = -lm -lpthread
```

### Multiple Source Files

```makefile
# For multiple C files
MODULES = pg_hello
OBJS = pg_hello.o helper.o utils.o

# Custom rules for each
pg_hello.o: src/pg_hello.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

helper.o: src/helper.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

utils.o: src/utils.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<
```

### Multiple Extensions

```makefile
# Build multiple extensions
EXTENSION = pg_hello pg_utils
MODULES = pg_hello pg_utils
DATA = sql/pg_hello--1.0.sql sql/pg_utils--1.0.sql
```

### Custom Test Configuration

```makefile
# Multiple test suites
REGRESS = basic advanced performance

# Custom test options
REGRESS_OPTS = --inputdir=test --load-extension=pg_hello
```

## 🐛 Troubleshooting

### Common Makefile Errors

#### 1. `pg_config: command not found`
```bash
# Solution: Add PostgreSQL to PATH
export PATH=/usr/pgsql-15/bin:$PATH

# Or specify full path
PG_CONFIG = /usr/pgsql-15/bin/pg_config
```

#### 2. `Permission denied` during install
```bash
# Solution: Use sudo
sudo make install

# Or install to custom location
make install DESTDIR=/tmp/pg_hello
```

#### 3. `No rule to make target`
```bash
# Check for typos in Makefile
# Ensure proper tabs (not spaces) for commands
# Verify file paths exist
```

#### 4. `undefined reference to` errors
```bash
# Add missing libraries
SHLIB_LINK = -lm

# Check PostgreSQL headers
pg_config --includedir
```

### Debugging Build Issues

#### Verbose Build
```bash
# See actual commands
make VERBOSE=1

# Or with detailed output
make V=1
```

#### Check Variables
```bash
# See what variables are set
make -p | grep ^PG

# Test pg_config
pg_config --version
pg_config --cflags
```

#### Manual Compilation
```bash
# Try compiling manually to debug
$(pg_config --includedir-server)
clang -I$(pg_config --includedir-server) -c src/pg_hello.c
```

### Best Practices

1. **Always run `make clean` before important builds**
2. **Test with `make installcheck` before releasing**
3. **Use version control for Makefile changes**
4. **Document custom variables and rules**
5. **Keep Makefile simple and readable**

## 📚 Further Reading

- [GNU Make Manual](https://www.gnu.org/software/make/manual/)
- [PostgreSQL Extension Building](https://www.postgresql.org/docs/current/extend-extensions.html)
- [PGXS Documentation](https://www.postgresql.org/docs/current/extend-pgxs.html)
- [PostgreSQL C Functions](https://www.postgresql.org/docs/current/xfunc-c.html)

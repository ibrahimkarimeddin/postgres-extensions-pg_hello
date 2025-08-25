# How PostgreSQL Extensions Work Internally

A deep dive into how PostgreSQL discovers, loads, and executes your extension code when you run `CREATE EXTENSION pg_hello`.

## 📋 Table of Contents

- [The Big Picture](#the-big-picture)
- [Step-by-Step Extension Loading](#step-by-step-extension-loading)
- [Function Registration Process](#function-registration-process)
- [Memory Layout and Loading](#memory-layout-and-loading)
- [Symbol Resolution](#symbol-resolution)
- [Runtime Execution Flow](#runtime-execution-flow)
- [Internal PostgreSQL Structures](#internal-postgresql-structures)
- [Debugging the Process](#debugging-the-process)

## 🔍 The Big Picture

When you run `CREATE EXTENSION pg_hello`, here's what happens behind the scenes:

```
1. PostgreSQL reads pg_hello.control
2. Finds and reads pg_hello--1.0.sql
3. Executes the SQL commands in that file
4. Each CREATE FUNCTION command registers your C functions
5. PostgreSQL maps SQL function names to C function symbols
6. When called, PostgreSQL loads the .so file and finds your functions
7. Your C code executes within PostgreSQL's process
```

Let's trace through each step in detail!

## 🔄 Step-by-Step Extension Loading

### Step 1: Control File Discovery

```bash
# When you run: CREATE EXTENSION pg_hello;
# PostgreSQL looks for:
ls $(pg_config --sharedir)/extension/pg_hello.control
```

**What PostgreSQL does:**
1. Searches the extension directory for `pg_hello.control`
2. Reads the metadata: version, dependencies, relocatable status
3. Determines which SQL file to execute

**Let's see what's in your control file:**
```ini
# pg_hello.control
comment = 'Tiny sample extension (C functions, GUC, SPI demo)'
default_version = '1.0'
relocatable = true
requires = ''
```

### Step 2: SQL Script Execution

PostgreSQL executes `pg_hello--1.0.sql`:

```sql
-- This is what PostgreSQL reads and executes:
CREATE FUNCTION pg_hello(text) RETURNS text
AS '$libdir/pg_hello', 'pg_hello'
LANGUAGE C STRICT IMMUTABLE;

CREATE FUNCTION now_ms() RETURNS bigint
AS '$libdir/pg_hello', 'now_ms'
LANGUAGE C STABLE;

CREATE FUNCTION spi_version() RETURNS text
AS '$libdir/pg_hello', 'spi_version'
LANGUAGE C STABLE;
```

**Key parts explained:**
- **`'$libdir/pg_hello'`** - Path to your shared library
- **`'pg_hello'`** - Name of the C function symbol
- **`LANGUAGE C`** - Tells PostgreSQL this is a C function
- **`STRICT`** - NULL inputs return NULL (no function call)
- **`IMMUTABLE/STABLE`** - Optimization hints for the query planner

## 🔗 Function Registration Process

### What Happens During CREATE FUNCTION

```sql
-- When PostgreSQL processes this line:
CREATE FUNCTION pg_hello(text) RETURNS text
AS '$libdir/pg_hello', 'pg_hello'
LANGUAGE C STRICT IMMUTABLE;
```

**PostgreSQL internal actions:**

1. **Parse the SQL statement**
2. **Register in system catalogs** (`pg_proc` table)
3. **Store the library path** (`$libdir/pg_hello`)
4. **Store the symbol name** (`pg_hello`)
5. **Store function metadata** (arguments, return type, properties)

Let's see this in action:

```sql
-- Check how your function is registered
SELECT 
    proname as function_name,
    probin as library_path,
    prosrc as symbol_name,
    provolatile as volatility,
    proisstrict as is_strict
FROM pg_proc 
WHERE proname IN ('pg_hello', 'now_ms', 'spi_version');
```

**Expected output:**
```
function_name | library_path    | symbol_name | volatility | is_strict
--------------+-----------------+-------------+------------+-----------
pg_hello      | $libdir/pg_hello| pg_hello    | i          | t
now_ms        | $libdir/pg_hello| now_ms      | s          | f  
spi_version   | $libdir/pg_hello| spi_version | s          | f
```

### The Symbol Registration Magic

In your C code, this line is crucial:

```c
PG_FUNCTION_INFO_V1(pg_hello);
```

**What this macro does:**
```c
// Expanded version of PG_FUNCTION_INFO_V1(pg_hello):
extern Datum pg_hello(PG_FUNCTION_ARGS);
extern const Pg_finfo_record * const pg_finfo_pg_hello(void);
const Pg_finfo_record * const pg_finfo_pg_hello(void) {
    static const Pg_finfo_record my_finfo = {
        1  // API version
    };
    return &my_finfo;
}
```

This creates a **symbol table entry** that PostgreSQL can find when loading your library.

## 🧠 Memory Layout and Loading

### Dynamic Library Loading

When someone calls your function for the first time:

```sql
SELECT pg_hello('World');
```

**PostgreSQL's process:**

1. **Check if library is loaded** - Is `pg_hello.so` already in memory?
2. **Load library if needed** - Use `dlopen()` to load `$libdir/pg_hello.so`
3. **Find function symbol** - Use `dlsym()` to find `pg_hello` symbol
4. **Verify function info** - Call `pg_finfo_pg_hello()` to check compatibility
5. **Cache function pointer** - Store for future calls
6. **Execute function** - Call your C code

### Memory Layout Visualization

```
PostgreSQL Process Memory:
┌─────────────────────────────────────┐
│ PostgreSQL Core                     │
├─────────────────────────────────────┤
│ Shared Libraries:                   │
│ ┌─────────────────────────────────┐ │
│ │ pg_hello.so                     │ │
│ │ ┌─────────────────────────────┐ │ │
│ │ │ pg_hello() function         │ │ │
│ │ │ now_ms() function           │ │ │
│ │ │ spi_version() function      │ │ │
│ │ │ _PG_init() initialization   │ │ │
│ │ └─────────────────────────────┘ │ │
│ └─────────────────────────────────┘ │
│ ┌─────────────────────────────────┐ │
│ │ Other extensions (.so files)    │ │
│ └─────────────────────────────────┘ │
├─────────────────────────────────────┤
│ Memory Contexts                     │
│ Stack                               │
│ Heap                                │
└─────────────────────────────────────┘
```

## 🔍 Symbol Resolution

### How PostgreSQL Finds Your Functions

Let's trace what happens when PostgreSQL looks for your `pg_hello` function:

```bash
# 1. PostgreSQL constructs the library path
# $libdir = /usr/lib/postgresql/15/lib (or similar)
# Full path: /usr/lib/postgresql/15/lib/pg_hello.so

# 2. Check if library exists
ls -la /usr/lib/postgresql/15/lib/pg_hello.so

# 3. Load library and examine symbols
nm -D /usr/lib/postgresql/15/lib/pg_hello.so | grep pg_hello
```

**Expected symbol output:**
```
0000000000001234 T pg_hello          # Your function
0000000000001567 T pg_finfo_pg_hello # Function info
```

### Symbol Table Details

Your compiled library contains these important symbols:

```c
// In your C code:
Datum pg_hello(PG_FUNCTION_ARGS)  // The actual function
{
    // Your code here
}

PG_FUNCTION_INFO_V1(pg_hello);    // Creates pg_finfo_pg_hello symbol
```

**What the linker creates:**
- **`pg_hello`** - Your actual function code
- **`pg_finfo_pg_hello`** - Function metadata for PostgreSQL
- **`_PG_init`** - Extension initialization (if present)

## ⚡ Runtime Execution Flow

### Complete Call Stack

When you execute `SELECT pg_hello('World')`:

```
1. SQL Parser
   ↓ (parses "SELECT pg_hello('World')")
2. Query Planner  
   ↓ (finds pg_hello function in pg_proc)
3. Function Manager
   ↓ (checks if library is loaded)
4. Dynamic Loader
   ↓ (loads pg_hello.so if needed)
5. Symbol Resolver
   ↓ (finds pg_hello symbol)
6. Function Call Interface
   ↓ (sets up PG_FUNCTION_ARGS)
7. Your C Function
   ↓ (executes your code)
8. Return Value Processing
   ↓ (converts result back to SQL)
9. Query Result
   (returns to client)
```

### Detailed Function Call Process

```c
// When PostgreSQL calls your function, it:

// 1. Sets up the function call info structure
FunctionCallInfoData fcinfo;
fcinfo.flinfo = &your_function_info;
fcinfo.nargs = 1;                    // Number of arguments
fcinfo.args[0] = text_value;         // Your 'World' argument
fcinfo.argnull[0] = false;           // Argument is not NULL

// 2. Calls your function
Datum result = pg_hello(&fcinfo);

// 3. Processes the result
if (fcinfo.isnull) {
    // Return NULL
} else {
    // Convert Datum to SQL value
    text *result_text = DatumGetTextP(result);
    // Return to client
}
```

### Your Function's Execution Context

When your C code runs, it has access to:

```c
Datum pg_hello(PG_FUNCTION_ARGS)
{
    // You have access to:
    // - PostgreSQL's memory contexts
    // - All PostgreSQL internal functions
    // - Database connection (for SPI)
    // - Configuration parameters
    // - Error handling system
    
    text *name = PG_GETARG_TEXT_PP(0);  // Get argument from PostgreSQL
    char *cname;                        // Your variables
    StringInfoData buf;                 // PostgreSQL's string builder
    text *result;                       // Return value
    
    // Your code executes in PostgreSQL's process space
    // with full access to the database
}
```

## 🏗️ Internal PostgreSQL Structures

### Function Cache Structure

PostgreSQL maintains an internal cache of loaded functions:

```c
// Simplified view of PostgreSQL's function cache
typedef struct FmgrInfo {
    PGFunction fn_addr;         // Pointer to your C function
    Oid fn_oid;                // Function OID from pg_proc
    short fn_nargs;            // Number of arguments
    bool fn_strict;            // Is function STRICT?
    bool fn_retset;            // Returns a set?
    unsigned char fn_stats;    // Usage statistics
    void *fn_extra;            // Extra info
    MemoryContext fn_mcxt;     // Memory context
    Node *fn_expr;             // Expression tree
} FmgrInfo;
```

### Extension Registry

PostgreSQL tracks extensions in these system tables:

```sql
-- Extension registry
SELECT * FROM pg_extension WHERE extname = 'pg_hello';

-- Extension objects (your functions)
SELECT 
    e.extname,
    p.proname,
    p.oid as function_oid
FROM pg_extension e
JOIN pg_depend d ON d.refobjid = e.oid  
JOIN pg_proc p ON p.oid = d.objid
WHERE e.extname = 'pg_hello' AND d.deptype = 'e';
```

## 🔧 Debugging the Process

### Trace Extension Loading

You can see what PostgreSQL is doing by enabling debug logging:

```sql
-- Enable function call logging
SET log_statement = 'all';
SET log_min_messages = 'debug1';

-- Drop and recreate extension to see the process
DROP EXTENSION IF EXISTS pg_hello;
CREATE EXTENSION pg_hello;

-- Check PostgreSQL logs to see the loading process
```

### Verify Library Loading

```sql
-- Check loaded libraries
SELECT * FROM pg_loaded_libraries() WHERE name LIKE '%pg_hello%';

-- Check function registration
\df pg_hello*

-- Verify function works
SELECT pg_hello('Debug Test');
```

### System Call Tracing

On Linux, you can trace system calls to see library loading:

```bash
# Trace a PostgreSQL backend process
sudo strace -p <postgres_pid> -e trace=openat,mmap -f

# In another terminal, trigger function call
psql -c "SELECT pg_hello('Trace Test');"
```

You'll see output like:
```
openat(AT_FDCWD, "/usr/lib/postgresql/15/lib/pg_hello.so", O_RDONLY|O_CLOEXEC) = 7
mmap(NULL, 16384, PROT_READ|PROT_EXEC, MAP_PRIVATE|MAP_DENYWRITE, 7, 0) = 0x7f...
```

### Function Call Statistics

Monitor your function's performance:

```sql
-- Reset statistics
SELECT pg_stat_reset();

-- Call your function multiple times
SELECT pg_hello('Test ' || i) FROM generate_series(1, 100) i;

-- Check statistics
SELECT 
    funcname,
    calls,
    total_time,
    mean_time,
    stddev_time
FROM pg_stat_user_functions 
WHERE funcname = 'pg_hello';
```

## 🧪 Hands-On Exploration

Let's trace through the entire process:

```sql
-- 1. Check if extension exists
SELECT * FROM pg_available_extensions WHERE name = 'pg_hello';

-- 2. See what files PostgreSQL will use
\! ls -la $(pg_config --sharedir)/extension/pg_hello*
\! ls -la $(pg_config --libdir)/pg_hello*

-- 3. Create extension and watch the registration
CREATE EXTENSION IF NOT EXISTS pg_hello;

-- 4. Examine how functions are registered
SELECT 
    p.proname,
    p.probin,
    p.prosrc,
    p.pronargs,
    format_type(p.prorettype, NULL) as return_type,
    array_to_string(
        array(
            SELECT format_type(unnest(p.proargtypes), NULL)
        ), 
        ', '
    ) as argument_types
FROM pg_proc p 
WHERE p.proname IN ('pg_hello', 'now_ms', 'spi_version');

-- 5. Test function calls and see them in action
SELECT pg_hello('Understanding PostgreSQL!');

-- 6. Check if library is loaded
SELECT name, setting FROM pg_settings WHERE name = 'dynamic_library_path';
```

## 🎯 Key Takeaways

**How PostgreSQL finds your code:**

1. **Control file** tells PostgreSQL what version and files to use
2. **SQL script** registers your C functions in the system catalog
3. **PG_FUNCTION_INFO_V1** creates discoverable symbols in your library
4. **Dynamic loading** loads your `.so` file when functions are first called
5. **Symbol resolution** maps SQL function names to C function pointers
6. **Function caching** keeps loaded functions in memory for performance

**The magic connections:**

- **SQL name ↔ C symbol**: `pg_hello` SQL function → `pg_hello` C function
- **Library path**: `$libdir/pg_hello` → `/usr/lib/postgresql/15/lib/pg_hello.so`
- **Function signature**: SQL types → C types via PostgreSQL's type system
- **Memory management**: Your C code runs in PostgreSQL's memory context

This is how PostgreSQL seamlessly integrates your C code into its SQL execution engine! 🚀

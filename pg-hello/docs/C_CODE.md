# C Code Structure and PostgreSQL API Guide

A comprehensive guide to understanding the C code implementation and PostgreSQL's C API.

## 📋 Table of Contents

- [Overview](#overview)
- [Code Structure](#code-structure)
- [PostgreSQL Headers](#postgresql-headers)
- [Function Anatomy](#function-anatomy)
- [Data Types and Conversion](#data-types-and-conversion)
- [Memory Management](#memory-management)
- [Error Handling](#error-handling)
- [GUC (Configuration) System](#guc-configuration-system)
- [SPI (Server Programming Interface)](#spi-server-programming-interface)
- [Best Practices](#best-practices)
- [Common Patterns](#common-patterns)

## 🔍 Overview

PostgreSQL extensions written in C have direct access to PostgreSQL's internal APIs, providing maximum performance and flexibility. Understanding the C API is crucial for effective extension development.

### Why C for PostgreSQL Extensions?

1. **Performance**: Native machine code execution
2. **Full API Access**: Complete access to PostgreSQL internals
3. **Memory Control**: Direct memory management
4. **System Integration**: Can call system functions and libraries
5. **Compatibility**: PostgreSQL core is written in C

## 🏗️ Code Structure

Let's analyze our `pg_hello.c` file section by section:

### 1. Header Includes

```c
#include "postgres.h"        // Core PostgreSQL definitions
#include "fmgr.h"           // Function manager interface
#include "utils/builtins.h" // Built-in utility functions
#include "utils/timestamp.h" // Timestamp functions
#include "executor/spi.h"   // Server Programming Interface
#include "miscadmin.h"      // Miscellaneous admin functions
#include "utils/guc.h"      // Grand Unified Configuration
```

**Purpose of each header**:
- **`postgres.h`**: Must be first, provides basic types and macros
- **`fmgr.h`**: Function manager, required for all PostgreSQL functions
- **`utils/builtins.h`**: Common utility functions like `text_to_cstring()`
- **`utils/timestamp.h`**: Timestamp manipulation functions
- **`executor/spi.h`**: Execute SQL from C code
- **`miscadmin.h`**: Administrative functions
- **`utils/guc.h`**: Configuration parameter management

### 2. Module Magic

```c
#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif
```

**Purpose**: 
- **Version Check**: Ensures extension is compatible with PostgreSQL version
- **Required**: All PostgreSQL extensions must include this
- **Safety**: Prevents loading incompatible extensions

**What it does**:
- Defines a magic number and version info
- PostgreSQL checks this when loading the extension
- Prevents crashes from version mismatches

### 3. Global Variables

```c
static int hello_repeat = 1;
```

**Purpose**: Store configuration values or shared state.

**Guidelines**:
- Use `static` for internal variables
- Initialize with safe defaults
- Use appropriate types (`int`, `bool`, `char*`)

### 4. Initialization Function

```c
void _PG_init(void);
void _PG_init(void)
{
    DefineCustomIntVariable(
        "pg_hello.repeat",      // Configuration name
        "How many times to repeat the greeting.",  // Description
        NULL,                   // Long description
        &hello_repeat,          // Variable to store value
        1,                      // Default value
        1,                      // Minimum value
        10,                     // Maximum value
        PGC_USERSET,           // Who can set it
        0,                      // Flags
        NULL, NULL, NULL       // Hook functions
    );
}
```

**Purpose**: Called when extension is loaded.

**Common uses**:
- Define configuration parameters
- Initialize global state
- Register hooks
- Set up shared memory

### 5. Function Declarations

```c
PG_FUNCTION_INFO_V1(pg_hello);
PG_FUNCTION_INFO_V1(now_ms);
PG_FUNCTION_INFO_V1(spi_version);
```

**Purpose**: Register functions with PostgreSQL's function manager.

**Required**: Every PostgreSQL function must have this declaration.

## 📁 PostgreSQL Headers

### Essential Headers

#### `postgres.h`
**Always include first!**
```c
#include "postgres.h"
```
Provides:
- Basic data types (`int32`, `int64`, `bool`)
- Memory allocation macros
- Porting macros for different platforms
- Core PostgreSQL definitions

#### `fmgr.h`
```c
#include "fmgr.h"
```
Provides:
- `PG_FUNCTION_ARGS` macro
- `PG_GETARG_*` macros for getting arguments
- `PG_RETURN_*` macros for returning values
- Function call interface

#### Common Utility Headers

```c
#include "utils/builtins.h"    // text_to_cstring(), cstring_to_text()
#include "utils/array.h"       // Array manipulation
#include "utils/lsyscache.h"   // System catalog lookups
#include "utils/memutils.h"    // Memory contexts
#include "catalog/pg_type.h"   // Type OIDs
#include "libpq/pqformat.h"    // Binary I/O functions
```

## 🔧 Function Anatomy

### Basic Function Structure

```c
Datum
function_name(PG_FUNCTION_ARGS)
{
    // 1. Variable declarations (at top for C90 compatibility)
    argument_type arg1;
    argument_type arg2;
    return_type result;
    
    // 2. Get arguments
    arg1 = PG_GETARG_TYPE(0);
    arg2 = PG_GETARG_TYPE(1);
    
    // 3. Check for NULL inputs (if not STRICT)
    if (PG_ARGISNULL(0))
        PG_RETURN_NULL();
    
    // 4. Function logic
    result = do_computation(arg1, arg2);
    
    // 5. Return result
    PG_RETURN_TYPE(result);
}
```

### Example: Our `pg_hello` Function

```c
Datum
pg_hello(PG_FUNCTION_ARGS)
{
    // 1. Declare all variables at top (C90 requirement)
    text *name = PG_GETARG_TEXT_PP(0);
    char *cname;
    StringInfoData buf;
    text *result;

    // 2. Convert PostgreSQL text to C string
    cname = text_to_cstring(name);

    // 3. Build result string with repeats
    initStringInfo(&buf);
    for (int i = 0; i < hello_repeat; i++)
    {
        if (i > 0) appendStringInfoString(&buf, " ");
        appendStringInfo(&buf, "Hello, %s!", cname);
    }

    // 4. Convert C string back to PostgreSQL text
    result = cstring_to_text(buf.data);
    
    // 5. Return the result
    PG_RETURN_TEXT_P(result);
}
```

### Function Signature Rules

1. **Return Type**: Always `Datum`
2. **Parameters**: Always `PG_FUNCTION_ARGS`
3. **Declaration**: Must have `PG_FUNCTION_INFO_V1(function_name)`
4. **Variables**: Declare at function start (C90 compatibility)

## 📊 Data Types and Conversion

### PostgreSQL to C Type Mapping

| PostgreSQL Type | C Type | Get Argument | Return Value |
|----------------|--------|--------------|--------------|
| `integer` | `int32` | `PG_GETARG_INT32(n)` | `PG_RETURN_INT32(x)` |
| `bigint` | `int64` | `PG_GETARG_INT64(n)` | `PG_RETURN_INT64(x)` |
| `boolean` | `bool` | `PG_GETARG_BOOL(n)` | `PG_RETURN_BOOL(x)` |
| `text` | `text*` | `PG_GETARG_TEXT_PP(n)` | `PG_RETURN_TEXT_P(x)` |
| `varchar` | `VarChar*` | `PG_GETARG_VARCHAR_PP(n)` | `PG_RETURN_VARCHAR_P(x)` |
| `float4` | `float4` | `PG_GETARG_FLOAT4(n)` | `PG_RETURN_FLOAT4(x)` |
| `float8` | `float8` | `PG_GETARG_FLOAT8(n)` | `PG_RETURN_FLOAT8(x)` |
| `timestamp` | `Timestamp` | `PG_GETARG_TIMESTAMP(n)` | `PG_RETURN_TIMESTAMP(x)` |

### Text Type Handling

Text is the most complex type to handle:

```c
// Getting text input
text *input_text = PG_GETARG_TEXT_PP(0);

// Convert to C string for processing
char *input_cstring = text_to_cstring(input_text);

// Process the string
char result_cstring[1024];
sprintf(result_cstring, "Processed: %s", input_cstring);

// Convert back to text for return
text *result_text = cstring_to_text(result_cstring);

// Return the text
PG_RETURN_TEXT_P(result_text);
```

### NULL Handling

```c
// Check if argument is NULL
if (PG_ARGISNULL(0))
    PG_RETURN_NULL();

// Return NULL
PG_RETURN_NULL();

// Functions marked STRICT automatically return NULL if any input is NULL
```

### Array Handling

```c
#include "utils/array.h"

Datum
process_array(PG_FUNCTION_ARGS)
{
    ArrayType *input_array;
    Datum *elements;
    bool *nulls;
    int nelems;
    
    // Get array argument
    input_array = PG_GETARG_ARRAYTYPE_P(0);
    
    // Deconstruct array
    deconstruct_array(input_array,
                     INT4OID,        // Element type OID
                     4,              // Element size
                     true,           // Element passed by value
                     'i',            // Element alignment
                     &elements,      // Output: element values
                     &nulls,         // Output: null flags
                     &nelems);       // Output: number of elements
    
    // Process elements...
    for (int i = 0; i < nelems; i++)
    {
        if (!nulls[i])
        {
            int32 value = DatumGetInt32(elements[i]);
            // Process value...
        }
    }
    
    // Return result...
}
```

## 🧠 Memory Management

PostgreSQL uses **memory contexts** for memory management:

### Memory Contexts

```c
#include "utils/memutils.h"

// Allocate memory in current context
char *buffer = palloc(1024);

// Allocate zero-initialized memory
char *buffer = palloc0(1024);

// Reallocate memory
buffer = repalloc(buffer, 2048);

// Free memory (optional - contexts are automatically cleaned up)
pfree(buffer);
```

### Important Memory Rules

1. **Use `palloc()`, not `malloc()`**
   - PostgreSQL manages the memory
   - Automatically freed when context is destroyed
   - Exception-safe

2. **Memory Contexts are Hierarchical**
   - Function calls create temporary contexts
   - Automatically cleaned up on function exit
   - Even if function throws an error

3. **StringInfo for Dynamic Strings**
   ```c
   StringInfoData buf;
   initStringInfo(&buf);
   appendStringInfo(&buf, "Number: %d", 42);
   appendStringInfoString(&buf, " more text");
   
   // buf.data contains the final string
   text *result = cstring_to_text(buf.data);
   ```

### Memory Context Example

```c
Datum
memory_example(PG_FUNCTION_ARGS)
{
    MemoryContext oldcontext;
    char *persistent_data;
    
    // Switch to a longer-lived context
    oldcontext = MemoryContextSwitchTo(TopMemoryContext);
    
    // This memory will survive function exit
    persistent_data = palloc(1024);
    
    // Switch back
    MemoryContextSwitchTo(oldcontext);
    
    // Regular palloc - cleaned up automatically
    char *temp_data = palloc(512);
    
    // Function logic...
    
    PG_RETURN_TEXT_P(cstring_to_text("Done"));
}
```

## ⚠️ Error Handling

PostgreSQL uses **exceptions** (longjmp/setjmp) for error handling:

### Reporting Errors

```c
#include "utils/elog.h"

// Fatal error (stops server)
ereport(FATAL,
        (errcode(ERRCODE_INTERNAL_ERROR),
         errmsg("fatal error occurred")));

// Error (aborts transaction)
ereport(ERROR,
        (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
         errmsg("invalid parameter: %s", param_name),
         errhint("parameter must be between 1 and 100")));

// Warning (continues execution)
ereport(WARNING,
        (errmsg("potential issue detected")));

// Notice (informational)
ereport(NOTICE,
        (errmsg("operation completed successfully")));

// Debug (only shown if log level allows)
ereport(DEBUG1,
        (errmsg("debug information: %d", debug_value)));
```

### Error Codes

```c
#include "utils/errcodes.h"

// Common error codes
ERRCODE_INVALID_PARAMETER_VALUE
ERRCODE_NULL_VALUE_NOT_ALLOWED
ERRCODE_DIVISION_BY_ZERO
ERRCODE_NUMERIC_VALUE_OUT_OF_RANGE
ERRCODE_INVALID_TEXT_REPRESENTATION
ERRCODE_FEATURE_NOT_SUPPORTED
```

### Exception Handling (TRY/CATCH)

```c
#include "utils/elog.h"

Datum
safe_operation(PG_FUNCTION_ARGS)
{
    bool success = false;
    
    PG_TRY();
    {
        // Risky operation that might throw error
        risky_function();
        success = true;
    }
    PG_CATCH();
    {
        // Handle the error
        ErrorData *edata = CopyErrorData();
        FlushErrorState();
        
        ereport(WARNING,
                (errmsg("operation failed: %s", edata->message)));
        
        FreeErrorData(edata);
    }
    PG_END_TRY();
    
    PG_RETURN_BOOL(success);
}
```

## ⚙️ GUC (Configuration) System

GUC stands for "Grand Unified Configuration" - PostgreSQL's parameter system.

### Defining Configuration Parameters

```c
// Global variable to store the value
static int my_parameter = 10;
static char *my_string_param = NULL;
static bool my_bool_param = false;

void _PG_init(void)
{
    // Integer parameter
    DefineCustomIntVariable(
        "myext.int_param",              // Parameter name
        "Description of the parameter", // Short description
        "Longer description...",        // Long description (can be NULL)
        &my_parameter,                  // Variable to store value
        10,                            // Default value
        1,                             // Minimum value
        1000,                          // Maximum value
        PGC_USERSET,                   // Who can set it
        0,                             // Flags
        NULL,                          // Check hook
        NULL,                          // Assign hook
        NULL                           // Show hook
    );
    
    // String parameter
    DefineCustomStringVariable(
        "myext.string_param",
        "A string parameter",
        NULL,
        &my_string_param,
        "default_value",
        PGC_USERSET,
        0,
        NULL, NULL, NULL
    );
    
    // Boolean parameter
    DefineCustomBoolVariable(
        "myext.bool_param",
        "A boolean parameter",
        NULL,
        &my_bool_param,
        false,                         // Default value
        PGC_USERSET,
        0,
        NULL, NULL, NULL
    );
}
```

### GUC Context Levels

| Level | Who Can Set | When Applied |
|-------|-------------|--------------|
| `PGC_POSTMASTER` | Only in config file, requires restart | Server start |
| `PGC_SIGHUP` | Config file or SIGHUP | Config reload |
| `PGC_SU_BACKEND` | Superuser, config file | Connection start |
| `PGC_BACKEND` | Config file, connection string | Connection start |
| `PGC_SUSET` | Superuser only | Anytime |
| `PGC_USERSET` | Any user | Anytime |

### Using Configuration Values

```c
Datum
use_config(PG_FUNCTION_ARGS)
{
    // Use the global variable
    int repeat_count = my_parameter;
    
    // Build result based on configuration
    StringInfoData buf;
    initStringInfo(&buf);
    
    for (int i = 0; i < repeat_count; i++)
    {
        appendStringInfo(&buf, "Iteration %d ", i);
    }
    
    PG_RETURN_TEXT_P(cstring_to_text(buf.data));
}
```

## 🖥️ SPI (Server Programming Interface)

SPI allows C code to execute SQL statements:

### Basic SPI Usage

```c
#include "executor/spi.h"

Datum
spi_example(PG_FUNCTION_ARGS)
{
    int ret;
    const char *query = "SELECT current_timestamp";
    
    // Connect to SPI
    if (SPI_connect() != SPI_OK_CONNECT)
        ereport(ERROR, (errmsg("SPI_connect failed")));
    
    // Execute query
    ret = SPI_execute(query, true, 1);  // true = read-only, 1 = max rows
    
    if (ret != SPI_OK_SELECT)
    {
        SPI_finish();
        ereport(ERROR, (errmsg("SPI_execute failed")));
    }
    
    // Process results
    if (SPI_processed > 0)
    {
        HeapTuple tuple = SPI_tuptable->vals[0];
        TupleDesc tupdesc = SPI_tuptable->tupdesc;
        
        bool isnull;
        Datum result_datum = SPI_getbinval(tuple, tupdesc, 1, &isnull);
        
        if (!isnull)
        {
            // Process the result...
        }
    }
    
    // Always disconnect
    SPI_finish();
    
    PG_RETURN_TEXT_P(cstring_to_text("Done"));
}
```

### SPI with Parameters

```c
Datum
spi_with_params(PG_FUNCTION_ARGS)
{
    int32 user_id = PG_GETARG_INT32(0);
    const char *query = "SELECT name FROM users WHERE id = $1";
    Oid argtypes[1] = {INT4OID};
    Datum values[1];
    char nulls[1] = {' '};
    
    values[0] = Int32GetDatum(user_id);
    
    if (SPI_connect() != SPI_OK_CONNECT)
        ereport(ERROR, (errmsg("SPI_connect failed")));
    
    int ret = SPI_execute_with_args(query, 1, argtypes, values, nulls, true, 1);
    
    // Process results...
    
    SPI_finish();
    PG_RETURN_TEXT_P(cstring_to_text("Result"));
}
```

### SPI Return Codes

```c
// Query execution results
SPI_OK_SELECT       // SELECT executed successfully
SPI_OK_INSERT       // INSERT executed successfully
SPI_OK_DELETE       // DELETE executed successfully
SPI_OK_UPDATE       // UPDATE executed successfully
SPI_OK_UTILITY      // Utility command executed

// Connection results
SPI_OK_CONNECT      // Connected successfully
SPI_OK_FINISH       // Disconnected successfully

// Error codes
SPI_ERROR_CONNECT   // Connection failed
SPI_ERROR_COPY      // COPY command not allowed
SPI_ERROR_CURSOR    // Cursor operation failed
```

## ✅ Best Practices

### 1. C90 Compatibility
```c
// WRONG - mixed declarations and code
Datum bad_function(PG_FUNCTION_ARGS)
{
    text *input = PG_GETARG_TEXT_PP(0);
    char *cstring = text_to_cstring(input);
    
    int len = strlen(cstring);  // Declaration after code
    
    // More code...
}

// CORRECT - all declarations at top
Datum good_function(PG_FUNCTION_ARGS)
{
    text *input;
    char *cstring;
    int len;
    
    input = PG_GETARG_TEXT_PP(0);
    cstring = text_to_cstring(input);
    len = strlen(cstring);
    
    // More code...
}
```

### 2. Error Handling
```c
// Always check for errors
if (SPI_connect() != SPI_OK_CONNECT)
    ereport(ERROR, (errmsg("SPI connection failed")));

// Provide helpful error messages
if (input_value < 0)
    ereport(ERROR,
            (errcode(ERRCODE_INVALID_PARAMETER_VALUE),
             errmsg("input value must be positive"),
             errhint("try using ABS() function")));
```

### 3. Memory Management
```c
// Use PostgreSQL memory functions
char *buffer = palloc(size);        // Not malloc()
buffer = repalloc(buffer, new_size); // Not realloc()
pfree(buffer);                       // Optional

// Use StringInfo for string building
StringInfoData buf;
initStringInfo(&buf);
appendStringInfo(&buf, "Value: %d", value);
```

### 4. NULL Handling
```c
// Check for NULL inputs
if (PG_ARGISNULL(0))
    PG_RETURN_NULL();

// Or mark function as STRICT in SQL
CREATE FUNCTION my_func(text) RETURNS text
AS '$libdir/my_ext', 'my_func'
LANGUAGE C STRICT;  -- Automatically returns NULL for NULL input
```

## 🔄 Common Patterns

### 1. Text Processing Function
```c
Datum
text_processor(PG_FUNCTION_ARGS)
{
    text *input;
    char *input_cstring;
    char *result_cstring;
    text *result;
    
    input = PG_GETARG_TEXT_PP(0);
    input_cstring = text_to_cstring(input);
    
    // Process the string
    result_cstring = process_string(input_cstring);
    
    result = cstring_to_text(result_cstring);
    PG_RETURN_TEXT_P(result);
}
```

### 2. Mathematical Function
```c
Datum
math_function(PG_FUNCTION_ARGS)
{
    float8 x, y, result;
    
    x = PG_GETARG_FLOAT8(0);
    y = PG_GETARG_FLOAT8(1);
    
    if (y == 0.0)
        ereport(ERROR,
                (errcode(ERRCODE_DIVISION_BY_ZERO),
                 errmsg("division by zero")));
    
    result = x / y;
    PG_RETURN_FLOAT8(result);
}
```

### 3. Database Query Function
```c
Datum
query_function(PG_FUNCTION_ARGS)
{
    const char *query;
    int ret;
    text *result;
    
    if (SPI_connect() != SPI_OK_CONNECT)
        ereport(ERROR, (errmsg("SPI_connect failed")));
    
    query = "SELECT current_database()";
    ret = SPI_execute(query, true, 1);
    
    if (ret == SPI_OK_SELECT && SPI_processed > 0)
    {
        char *db_name = SPI_getvalue(SPI_tuptable->vals[0], 
                                    SPI_tuptable->tupdesc, 1);
        result = cstring_to_text(db_name);
    }
    else
    {
        result = cstring_to_text("unknown");
    }
    
    SPI_finish();
    PG_RETURN_TEXT_P(result);
}
```

This comprehensive guide should help you understand and work with PostgreSQL's C API effectively!

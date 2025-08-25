#include "postgres.h"
#include "fmgr.h"
#include "utils/builtins.h"
#include "utils/timestamp.h"
#include "executor/spi.h"
#include "miscadmin.h"
#include "utils/guc.h"

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

// --- GUC (custom setting) example ---
static int hello_repeat = 1;

void _PG_init(void);
void _PG_init(void)
{
    DefineCustomIntVariable(
        "pg_hello.repeat",
        "How many times to repeat the greeting.",
        NULL,
        &hello_repeat,
        1,  // default
        1,  // min
        10, // max
        PGC_USERSET,
        0,
        NULL, NULL, NULL
    );
}

// --- Function declarations ---
PG_FUNCTION_INFO_V1(pg_hello);
PG_FUNCTION_INFO_V1(now_ms);
PG_FUNCTION_INFO_V1(spi_version);

// text pg_hello(text name)
Datum
pg_hello(PG_FUNCTION_ARGS)
{
    text *name = PG_GETARG_TEXT_PP(0);

    // Convert to C string
    char *cname = text_to_cstring(name);

    // Build result string with repeats
    StringInfoData buf;
    initStringInfo(&buf);
    for (int i = 0; i < hello_repeat; i++)
    {
        if (i > 0) appendStringInfoString(&buf, " ");
        appendStringInfo(&buf, "Hello, %s!", cname);
    }

    text *result = cstring_to_text(buf.data);
    PG_RETURN_TEXT_P(result);
}

// bigint now_ms()
Datum
now_ms(PG_FUNCTION_ARGS)
{
    TimestampTz now = GetCurrentTimestamp();        // microseconds
    // Convert to milliseconds since Postgres epoch; easier via timestamptz_to_time_t + math.
    // But a simple way: use integer division on microseconds.
    long long micros = (long long) now;             // microseconds
    long long millis = micros / 1000LL;
    PG_RETURN_INT64((int64) millis);
}

// text spi_version()
Datum
spi_version(PG_FUNCTION_ARGS)
{
    if (SPI_connect() != SPI_OK_CONNECT)
        ereport(ERROR, (errmsg("SPI_connect failed")));

    const char *query = "SELECT version()";
    int ret = SPI_execute(query, true, 1);
    if (ret != SPI_OK_SELECT || SPI_processed != 1)
    {
        SPI_finish();
        ereport(ERROR, (errmsg("SPI_execute failed")));
    }

    HeapTuple tuple = SPI_tuptable->vals[0];
    TupleDesc tupdesc = SPI_tuptable->tupdesc;

    bool isnull = false;
    Datum d = SPI_getbinval(tuple, tupdesc, 1, &isnull);

    text *result = isnull ? cstring_to_text("NULL") : DatumGetTextPP(d);

    SPI_finish();
    PG_RETURN_TEXT_P(cstring_to_text(text_to_cstring(result)));
}

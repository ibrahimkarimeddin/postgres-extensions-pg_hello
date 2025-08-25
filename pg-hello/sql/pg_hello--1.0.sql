-- Objects created by this version
CREATE FUNCTION pg_hello(text) RETURNS text
AS '$libdir/pg_hello', 'pg_hello'
LANGUAGE C STRICT IMMUTABLE;

-- Returns current_timestamp in milliseconds as BIGINT
CREATE FUNCTION now_ms() RETURNS bigint
AS '$libdir/pg_hello', 'now_ms'
LANGUAGE C STABLE;

-- Run a simple SELECT via SPI to show server-side SQL execution
CREATE FUNCTION spi_version() RETURNS text
AS '$libdir/pg_hello', 'spi_version'
LANGUAGE C STABLE;
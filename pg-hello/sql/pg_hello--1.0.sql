-- Objects created by this version
CREATE FUNCTION pg_hello(text) RETURNS text
AS 'MODULE_PATHNAME', 'pg_hello'
LANGUAGE C STRICT IMMUTABLE;

-- Returns current_timestamp in milliseconds as BIGINT
CREATE FUNCTION now_ms() RETURNS bigint
AS 'MODULE_PATHNAME', 'now_ms'
LANGUAGE C STABLE;

-- Run a simple SELECT via SPI to show server-side SQL execution
CREATE FUNCTION spi_version() RETURNS text
AS 'MODULE_PATHNAME', 'spi_version'
LANGUAGE C STABLE;
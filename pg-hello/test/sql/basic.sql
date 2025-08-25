CREATE EXTENSION pg_hello;

SET pg_hello.repeat = 2;
SELECT pg_hello('Suleiman') as greet;
SELECT now_ms() > 0 as ok;
SELECT spi_version() like 'PostgreSQL%' as has_ver;

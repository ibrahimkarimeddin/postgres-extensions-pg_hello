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
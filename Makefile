# contrib/replace_empty_string/Makefile

MODULES = replace_empty_string

EXTENSION = replace_empty_string

DATA = replace_empty_string--1.0.sql

REGRESS_OPTS = --dbname=$(PL_TESTDB)
REGRESS = replace_empty_string

ifdef NO_PGXS
subdir = contrib/replace_empty_string
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
else
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
endif

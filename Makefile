MODULE_big = fasttrun
OBJS = fasttrun.o	
DATA = fasttrun--2.0.sql fasttrun--unpackaged--2.0.sql
DOCS = README.fasttrun
REGRESS = fasttrun
EXTENSION=fasttrun


ifdef USE_PGXS
PGXS := $(shell pg_config --pgxs)
include $(PGXS)
else
subdir = contrib/fasttrun
top_builddir = ../..
#include $(top_builddir)/src/Makefile.global
#include $(top_srcdir)/contrib/contrib-global.mk
endif

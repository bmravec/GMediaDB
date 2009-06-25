lib_LTLIBRARIES = gmediadb.la

gmediadb_la_SOURCES=gmediadb.h gmediadb.c

library_includedir=$(includedir)/
library_include_HEADERS=gmediadb.h

gmediadb_la_LDFLAGS=$(PROG_CFLAGS)
gmediadb_la_LIBADD=$(PROG_LIBS)
gmediadb_la_DEPENDENCIES=$(PROG)

pkgconfigdir=$(libdir)/pkgconfig
pkgconfig_DATA=gmediadb-1.0.pc

EXTRA_DIST=gmediadb-1.0.pc.in
CLEANFILES=gmediadb-1.0.pc

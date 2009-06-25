lib_LTLIBRARIES = libgmediadb.la

libgmediadb_la_SOURCES=gmediadb.h gmediadb.c

library_includedir=$(includedir)/
library_include_HEADERS=gmediadb.h

libgmediadb_la_LDFLAGS=$(PROG_CFLAGS)
libgmediadb_la_LIBADD=$(PROG_LIBS)
libgmediadb_la_DEPENDENCIES=$(PROG)

pkgconfigdir=$(libdir)/pkgconfig
pkgconfig_DATA=gmediadb-1.0.pc

EXTRA_DIST=gmediadb-1.0.pc.in
CLEANFILES=gmediadb-1.0.pc

INCLUDES=$(PROG_CFLAGS)

lib_LTLIBRARIES = libgmediadb.la

libgmediadb_la_SOURCES=gmediadb.h gmediadb.c

library_includedir=$(includedir)/
library_include_HEADERS=gmediadb.h

libgmediadb_la_LDFLAGS=$(PROG_CFLAGS)
libgmediadb_la_LIBADD=$(PROG_LIBS)
libgmediadb_la_DEPENDENCIES=$(PROG)

pkgconfigdir=$(libdir)/pkgconfig
pkgconfig_DATA=gmediadb.pc

EXTRA_DIST=gmediadb.pc.in
CLEANFILES=gmediadb.pc

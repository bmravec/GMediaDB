INCLUDES=$(GLIB_CFLAGS) $(DBUS_CFLAGS) $(SQLITE_CFLAGS)

lib_LTLIBRARIES = libgmediadb.la

libgmediadb_la_SOURCES=gmediadb.h gmediadb.c

library_includedir=$(includedir)/
library_include_HEADERS=gmediadb.h

libgmediadb_la_LDFLAGS=$(GLIB_CFLAGS) $(DBUS_CFLAGS) $(SQLITE_CFLAGS)
libgmediadb_la_LIBADD=$(GLIB_LIBS) $(DBUS_LIBS) $(SQLITE_LIBS)
libgmediadb_la_DEPENDENCIES=$(PROG)

pkgconfigdir=$(libdir)/pkgconfig
pkgconfig_DATA=gmediadb.pc

EXTRA_DIST=gmediadb.pc.in
CLEANFILES=gmediadb.pc

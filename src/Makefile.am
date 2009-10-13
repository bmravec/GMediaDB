media-object-glue.h: media-object.xml
	$(DBUSBINDINGTOOL) --mode=glib-server --output=$@ --prefix=media_object $^

INCLUDES=$(GLIB_CFLAGS) $(DBUS_CFLAGS)

lib_LTLIBRARIES=libgmediadb.la

libgmediadb_la_SOURCES=             \
    gmediadb.c gmediadb.h           \
    media-object.c media-object.h   \
    media-object-glue.h

library_includedir=$(includedir)/
library_include_HEADERS=gmediadb.h

libgmediadb_la_LDFLAGS=$(GLIB_CFLAGS) $(DBUS_CFLAGS)
libgmediadb_la_LIBADD=$(GLIB_LIBS) $(DBUS_LIBS)
libgmediadb_la_DEPENDENCIES=$(PROG)

BUILT_SOURCES=media-object-glue.h

pkgconfigdir=$(libdir)/pkgconfig
pkgconfig_DATA=gmediadb.pc

EXTRA_DIST=gmediadb.pc.in media-object.xml
CLEANFILES=gmediadb.pc media-object-glue.h

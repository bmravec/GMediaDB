servicedir = $(datadir)/dbus-1/services
service_in_files = org.gnome.GMediaDB.service.in
service_DATA = org.gnome.GMediaDB.service

org.gnome.GMediaDB.service: org.gnome.GMediaDB.service.in
	sed -e "s|\@bindir\@|$(bindir)|" $< > $@

gmediadb-glue.h: gmediadb.xml
	$(DBUSBINDINGTOOL) --mode=glib-server --output=$@ --prefix=gmediadb $^

media-object-glue.h: media-object.xml
	$(DBUSBINDINGTOOL) --mode=glib-server --output=$@ --prefix=media_object $^

INCLUDES = $(GLIB_CFLAGS) $(DBUS_CFLAGS)

bin_PROGRAMS = gmediadb

BUILT_SOURCES = gmediadb-glue.h media-object-glue.h

gmediadb_SOURCES = \
	gmediadb.c gmediadb.h gmediadb-glue.h \
	media-object.c media-object.h media-object-glue.h

gmediadb_LDADD = $(GLIB_LIBS) $(DBUS_LIBS)

EXTRA_DIST = gmediadb.xml media-object.xml $(service_in_files)
CLEANFILES = gmediadb-glue.h $(service_DATA)

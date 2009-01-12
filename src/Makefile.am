INCLUDES = $(PROG_CFLAGS)

gmediadb-marshal.c: gmediadb-marshal.list
	$(GLIB_GENMARSHAL) --prefix=gmediadb_marshal gmediadb-marshal.list --body > gmediadb-marshal.c

gmediadb-marshal.h: gmediadb-marshal.list
	$(GLIB_GENMARSHAL) --prefix=gmediadb_marshal gmediadb-marshal.list --header > gmediadb-marshal.h

media-object-glue.h: media-object.xml
	$(DBUSBINDINGTOOL) --mode=glib-server --output=$@ --prefix=media_object $^

bin_PROGRAMS = gmediadb gmediadb-util

BUILT_SOURCES = \
	media-object-glue.h \
	gmediadb-marshal.c \
	gmediadb-marshal.h

gmediadb_SOURCES = \
	gmediadb-marshal.c gmediadb-marshal.h \
	gmediadb.c gmediadb.h \
	media-object.c media-object.h media-object-glue.h \
	tag-handler.c tag-handler.h
        
gmediadb_LDADD = $(PROG_LIBS)

gmediadb_util_SOURCES = gmediadb-util.c
gmediadb_util_LDADD = $(PROG_LIBS)

EXTRA_DIST = \
	gmediadb-marshal.list \
	media-object.xml

CLEANFILES = \
	media-object-glue.h \
	gmediadb-marshal.h \
	gmediadb-marshal.c

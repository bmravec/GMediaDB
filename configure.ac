AC_PREREQ(2.59)
AC_INIT([gmediadb],[0.5.0])
AC_CONFIG_SRCDIR([gmediadb/gmediadb.c])
AM_INIT_AUTOMAKE([dist-bzip2])
CFLAGS="$CFLAGS -g"
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_LIBTOOL

AC_CONFIG_MACRO_DIR([m4])

DBUS_REQUIRED=0.60

PKG_CHECK_MODULES(PROG, glib-2.0 gthread-2.0 dbus-1 >= 0.60 dbus-glib-1 >= 0.60)
AC_SUBST(PROG_CFLAGS)
AC_SUBST(PROG_LIBS)

AC_PATH_PROG(DBUSBINDINGTOOL, dbus-binding-tool)
AC_SUBST(DBUSBINDINGTOOL)

AC_OUTPUT([
Makefile
gmediadb/Makefile
gmediadb/gmediadb-1.0.pc
dbus-server/Makefile
])

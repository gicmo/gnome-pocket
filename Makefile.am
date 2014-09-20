AUTOMAKE_OPTIONS=subdir-objects
NULL =
CLEANFILES =
MAINTAINERCLEANFILES =
EXTRA_DIST =
noinst_DATA =

AM_CPPFLAGS =					\
        -I$(top_srcdir)				\
        -DPREFIX=\"$(prefix)\"			\
        -DLIBDIR=\"$(libdir)\"			\
        -DG_LOG_DOMAIN=\"gnome-pocket\"		\
        -DG_DISABLE_DEPRECATED			\
        $(GNOME_POCKET_CFLAGS)				\
        $(NULL)

noinst_PROGRAMS =

if GNOME_POCKET_STATIC
noinst_LTLIBRARIES = libgnome-pocket.la
else
pkglib_LTLIBRARIES = libgnome-pocket.la
endif

libgnome_pocket_la_LIBADD = $(GNOME_POCKET_LIBS)
libgnome_pocket_la_LDFLAGS = -avoid-version
libgnome_pocket_la_SOURCES =				\
	gnome-pocket/gnome-pocket.h			\
	gnome-pocket/gnome-pocket.c			\
	gnome-pocket/gnome-pocket-item.h		\
	gnome-pocket/gnome-pocket-item.c

noinst_PROGRAMS += test
test_SOURCES =					\
	test.c					\
	$(NULL)
test_LDADD =					\
	$(GNOME_POCKET_LIBS)			\
	libgnome-pocket.la			\
	$(NULL)

if GNOME_POCKET_GIR
include $(INTROSPECTION_MAKEFILE)
INTROSPECTION_GIRS = GnomePocket-1.0.gir

GnomePocket-1.0.gir: libgnome-pocket.la Makefile
GnomePocket_1_0_gir_NAMESPACE = GnomePocket
GnomePocket_1_0_gir_VERSION = 1.0
GnomePocket_1_0_gir_LIBS = libgnome-pocket.la
GnomePocket_1_0_gir_CFLAGS = $(AM_CPPFLAGS)
GnomePocket_1_0_gir_SCANNERFLAGS =			\
       --warn-all					\
       --symbol-prefix=gnome				\
       --identifier-prefix=Gnome			\
        --c-include="gnome-pocket/gnome-pocket.h"	\
       $(NULL)
GnomePocket_1_0_gir_INCLUDES = $(GNOME_POCKET_GIR_INCLUDES)
GnomePocket_1_0_gir_FILES = $(libgnome_pocket_la_SOURCES)

if GNOME_POCKET_STATIC
noinst_DATA += $(srcdir)/GnomePocket-1.0.gir
EXTRA_DIST += $(srcdir)/GnomePocket-1.0.gir
MAINTAINERCLEANFILES += $(srcdir)/GnomePocket-1.0.gir
else
girdir= $(pkgdatadir)/gir-1.0
typelibdir= $(pkglibdir)/girepository-1.0

gir_DATA = $(INTROSPECTION_GIRS)
typelib_DATA = $(gir_DATA:.gir=.typelib)

CLEANFILES += $(gir_DATA) $(typelib_DATA)
endif
endif

-include $(top_srcdir)/git.mk

## Process this file with automake to produce Makefile.in
PFLAGS=-module -avoid-version -no-undefined

libmousegestures_la_LDFLAGS = $(PFLAGS)
libmousegestures_la_LIBADD = @COMPIZ_LIBS@
nodist_libmousegestures_la_SOURCES = mousegestures_options.c mousegestures_options.h
dist_libmousegestures_la_SOURCES = mousegestures.c

BUILT_SOURCES = $(nodist_libmousegestures_la_SOURCES)

AM_CPPFLAGS =                              \
	-I$(top_srcdir)/include             \
	@COMPIZ_CFLAGS@                     \
	-DDATADIR='"$(compdatadir)"'        \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"'

moduledir = $(plugindir)

module_LTLIBRARIES = mousegestures.la

CLEANFILES = *_options.c *_options.h

mousegestures_options.h: ../../metadata/mousegestures.xml.in
		$(BCOP_BIN) --header $@ $<

mousegestures_options.c: ../../metadata/mousegestures.xml.in mousegestures_options.h
		$(BCOP_BIN) --source $@ $<

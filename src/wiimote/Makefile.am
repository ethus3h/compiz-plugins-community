## Process this file with automake to produce Makefile.in
PFLAGS=-module -avoid-version -no-undefined

libwiimote_la_LDFLAGS = $(PFLAGS)
libwiimote_la_LIBADD = @COMPIZ_LIBS@
nodist_libwiimote_la_SOURCES = wiimote_options.c wiimote_options.h
dist_libwiimote_la_SOURCES = action.c callback.c communicate.c compiz-wiimote.h connect.c gesture.c option.c report.c util.c util.c wiimote.c

BUILT_SOURCES = $(nodist_libwiimote_la_SOURCES)

AM_CPPFLAGS =                              \
	-I$(top_srcdir)/include             \
	@COMPIZ_CFLAGS@                     \
	-DDATADIR='"$(compdatadir)"'        \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"'          \
	-std=c99

moduledir = $(plugindir)

module_LTLIBRARIES = libwiimote.la

CLEANFILES = *_options.c *_options.h

wiimote_options.h: ../../metadata/wiimote.xml.in
		$(BCOP_BIN) --header $@ $<

wiimote_options.c: ../../metadata/wiimote.xml.in wiimote_options.h
		$(BCOP_BIN) --source $@ $<

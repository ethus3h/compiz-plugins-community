
## Process this file with automake to produce Makefile.in
PFLAGS=-module -avoid-version -no-undefined

libvisualevent_la_LDFLAGS = $(PFLAGS)
libvisualevent_la_LIBADD = @COMPIZ_LIBS@
nodist_libvisualevent_la_SOURCES = visualevent_options.c visualevent_options.h
dist_libvisualevent_la_SOURCES = compiz-cairo.c compiz-cairo.h visualevent.c

BUILT_SOURCES = $(nodist_libvisualevent_la_SOURCES)

AM_CPPFLAGS =                              \
	-I$(top_srcdir)/include             \
	@COMPIZ_CFLAGS@                     \
	-DDATADIR='"$(compdatadir)"'        \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"'

moduledir = $(plugindir)

module_LTLIBRARIES = libvisualevent.la

CLEANFILES = *_options.c *_options.h

visualevent_options.h: ../../metadata/visualevent.xml.in
		$(BCOP_BIN) --header $@ $<

visualevent_options.c: ../../metadata/visualevent.xml.in visualevent_options.h
		$(BCOP_BIN) --source $@ $<

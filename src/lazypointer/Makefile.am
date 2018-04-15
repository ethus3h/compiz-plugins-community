## Process this file with automake to produce Makefile.in
PFLAGS=-module -avoid-version -no-undefined

liblazypointer_la_LDFLAGS = $(PFLAGS)
liblazypointer_la_LIBADD = @COMPIZ_LIBS@
nodist_liblazypointer_la_SOURCES = lazypointer_options.c lazypointer_options.h
dist_liblazypointer_la_SOURCES = lazypointer.c

BUILT_SOURCES = $(nodist_liblazypointer_la_SOURCES)

AM_CPPFLAGS =                              \
	-I$(top_srcdir)/include             \
	@COMPIZ_CFLAGS@                     \
	-DDATADIR='"$(compdatadir)"'        \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"'

moduledir = $(plugindir)

module_LTLIBRARIES = liblazypointer.la

CLEANFILES = *_options.c *_options.h

lazypointer_options.h: ../../metadata/lazypointer.xml.in
		$(BCOP_BIN) --header $@ $<

lazypointer_options.c: ../../metadata/lazypointer.xml.in lazypointer_options.h
		$(BCOP_BIN) --source $@ $<

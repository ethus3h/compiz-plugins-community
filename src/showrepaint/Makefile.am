## Process this file with automake to produce Makefile.in
PFLAGS=-module -avoid-version -no-undefined

libshowrepaint_la_LDFLAGS = $(PFLAGS)
libshowrepaint_la_LIBADD = @COMPIZ_LIBS@
nodist_libshowrepaint_la_SOURCES = showrepaint_options.c showrepaint_options.h
dist_libshowrepaint_la_SOURCES = showrepaint.c

BUILT_SOURCES = $(nodist_libshowrepaint_la_SOURCES)

AM_CPPFLAGS =                              \
	-I$(top_srcdir)/include             \
	@COMPIZ_CFLAGS@                     \
	-DDATADIR='"$(compdatadir)"'        \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"'

moduledir = $(plugindir)

module_LTLIBRARIES = libshowrepaint.la

CLEANFILES = *_options.c *_options.h

showrepaint_options.h: ../../metadata/showrepaint.xml.in
		$(BCOP_BIN) --header $@ $<

showrepaint_options.c: ../../metadata/showrepaint.xml.in showrepaint_options.h
		$(BCOP_BIN) --source $@ $<

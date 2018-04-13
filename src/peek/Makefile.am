## Process this file with automake to produce Makefile.in
PFLAGS=-module -avoid-version -no-undefined

libpeek_la_LDFLAGS = $(PFLAGS)
libpeek_la_LIBADD = @COMPIZ_LIBS@
nodist_libpeek_la_SOURCES = peek_options.c peek_options.h
dist_libpeek_la_SOURCES = peek.c

BUILT_SOURCES = $(nodist_libmaze_la_SOURCES)

AM_CPPFLAGS =                              \
	-I$(top_srcdir)/include             \
	@COMPIZ_CFLAGS@                     \
	-DDATADIR='"$(compdatadir)"'        \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"'

moduledir = $(plugindir)

module_LTLIBRARIES = libpeek.la

CLEANFILES = *_options.c *_options.h

peek_options.h: ../../metadata/peek.xml.in
		$(BCOP_BIN) --header $@ $<

peek_options.c: ../../metadata/peek.xml.in peek_options.h
		$(BCOP_BIN) --source $@ $<

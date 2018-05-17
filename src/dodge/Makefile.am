## Process this file with automake to produce Makefile.in
PFLAGS=-module -avoid-version -no-undefined

libdodge_la_LDFLAGS = $(PFLAGS)
libdodge_la_LIBADD = @COMPIZ_LIBS@
nodist_libdodge_la_SOURCES = dodge_options.c dodge_options.h
dist_libdodge_la_SOURCES = dodge.c

BUILT_SOURCES = $(nodist_libdodge_la_SOURCES)

AM_CPPFLAGS =                              \
	-I$(top_srcdir)/include             \
	@COMPIZ_CFLAGS@                     \
	-DDATADIR='"$(compdatadir)"'        \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"'

moduledir = $(plugindir)

module_LTLIBRARIES = libdodge.la

CLEANFILES = *_options.c *_options.h

dodge_options.h: ../../metadata/dodge.xml.in
		$(BCOP_BIN) --header $@ $<

dodge_options.c: ../../metadata/dodge.xml.in dodge_options.h
		$(BCOP_BIN) --source $@ $<

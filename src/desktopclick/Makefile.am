## Process this file with automake to produce Makefile.in
PFLAGS=-module -avoid-version -no-undefined

libdclick_la_LDFLAGS = $(PFLAGS)
libdclick_la_LIBADD = @COMPIZ_LIBS@
nodist_libdclick_la_SOURCES = dclick_options.c dclick_options.h
dist_libdclick_la_SOURCES = dclick.c

BUILT_SOURCES = $(nodist_libdclick_la_SOURCES)

AM_CPPFLAGS =                              \
	-I$(top_srcdir)/include             \
	@COMPIZ_CFLAGS@                     \
	-DDATADIR='"$(compdatadir)"'        \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"'

moduledir = $(plugindir)

module_LTLIBRARIES = libdclick.la

CLEANFILES = *_options.c *_options.h

dclick_options.h: ../../metadata/dclick.xml.in
		$(BCOP_BIN) --header $@ $<

dclick_options.c: ../../metadata/dclick.xml.in dclick_options.h
		$(BCOP_BIN) --source $@ $<

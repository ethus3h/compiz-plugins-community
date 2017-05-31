## Process this file with automake to produce Makefile.in
PFLAGS=-module -avoid-version -no-undefined

librubik_la_LDFLAGS = $(PFLAGS)
librubik_la_LIBADD = @COMPIZ_LIBS@
nodist_librubik_la_SOURCES = rubik_options.c rubik_options.h
dist_librubik_la_SOURCES = rubik.c

BUILT_SOURCES = $(nodist_librubik_la_SOURCES)

AM_CPPFLAGS =                              \
	-I$(top_srcdir)/include             \
	@COMPIZ_CFLAGS@                     \
	-DDATADIR='"$(compdatadir)"'        \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"'

moduledir = $(plugindir)

module_LTLIBRARIES = librubik.la

CLEANFILES = *_options.c *_options.h

rubik_options.h: ../../metadata/rubik.xml.in
		$(BCOP_BIN) --header $@ $<

rubik_options.c: ../../metadata/rubik.xml.in rubik_options.h
		$(BCOP_BIN) --source $@ $<

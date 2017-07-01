## Process this file with automake to produce Makefile.in
PFLAGS=-module -avoid-version -no-undefined

libmaze_la_LDFLAGS = $(PFLAGS)
libmaze_la_LIBADD = @COMPIZ_LIBS@
nodist_libmaze_la_SOURCES = maze_options.c maze_options.h
dist_libmaze_la_SOURCES = maze.c

BUILT_SOURCES = $(nodist_libmaze_la_SOURCES)

AM_CPPFLAGS =                              \
	-I$(top_srcdir)/include             \
	@COMPIZ_CFLAGS@                     \
	-DDATADIR='"$(compdatadir)"'        \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"'

moduledir = $(plugindir)

module_LTLIBRARIES = libmaze.la

CLEANFILES = *_options.c *_options.h

maze_options.h: ../../metadata/maze.xml.in
		$(BCOP_BIN) --header $@ $<

maze_options.c: ../../metadata/maze.xml.in maze_options.h
		$(BCOP_BIN) --source $@ $<

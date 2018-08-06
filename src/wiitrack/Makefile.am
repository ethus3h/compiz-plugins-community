## Process this file with automake to produce Makefile.in
PFLAGS=-module -avoid-version -no-undefined

if WIITRACK_PLUGIN
libwiitrack_la_LDFLAGS = $(PFLAGS)
libwiitrack_la_LIBADD = @COMPIZ_LIBS@ @CWIID_LIBS@
nodist_libwiitrack_la_SOURCES = wiitrack_options.c wiitrack_options.h
dist_libwiitrack_la_SOURCES = wiitrack.c compiz-wiimote.h
endif

BUILT_SOURCES = $(nodist_libwiitrack_la_SOURCES)

AM_CPPFLAGS =                              \
	-I$(top_srcdir)/include             \
	@COMPIZ_CFLAGS@                     \
	-DDATADIR='"$(compdatadir)"'        \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"'
	
moduledir = $(plugindir)

if WIITRACK_PLUGIN
module_LTLIBRARIES = libwiitrack.la
endif

CLEANFILES = *_options.c *_options.h

wiitrack_options.h: ../../metadata/wiitrack.xml.in
		$(BCOP_BIN) --header $@ $<

wiitrack_options.c: ../../metadata/wiitrack.xml.in wiitrack_options.h
		$(BCOP_BIN) --source $@ $<

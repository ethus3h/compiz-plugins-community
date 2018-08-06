## Process this file with automake to produce Makefile.in
PFLAGS=-module -avoid-version -no-undefined

if WIIMOTE_PLUGIN
libwiimote_la_LDFLAGS = $(PFLAGS)
libwiimote_la_LIBADD = @COMPIZ_LIBS@ @CWIID_LIBS@
nodist_libwiimote_la_SOURCES = wiimote_options.c  \
			        wiimote_options.h
dist_libwiimote_la_SOURCES = \
	action.c  \
 	callback.c  \
	communicate.c  \
	compiz-wiimote.h  \
	connect.c  \
	gesture.c  \
	option.c  \
	report.c  \
	util.c  \
	wiimote-internal.h  \
	wiimote.c
endif

BUILT_SOURCES = $(nodist_libwiimote_la_SOURCES)

AM_CPPFLAGS =                              \
	-I$(top_srcdir)/include             \
	@COMPIZ_CFLAGS@                     \
	-DDATADIR='"$(compdatadir)"'        \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"' 

LDFLAGS= -lcwiid

moduledir = $(plugindir)

if WIIMOTE_PLUGIN
module_LTLIBRARIES = libwiimote.la
endif

CLEANFILES = *_options.c *_options.h

wiimote_options.h: ../../metadata/wiimote.xml.in
		$(BCOP_BIN) --header $@ $<

wiimote_options.c: ../../metadata/wiimote.xml.in wiimote_options.h
		$(BCOP_BIN) --source $@ $<

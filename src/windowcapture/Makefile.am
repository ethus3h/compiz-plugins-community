## Process this file with automake to produce Makefile.in
PFLAGS=-module -avoid-version -no-undefined

libwindowcapture_la_LDFLAGS = $(PFLAGS)
libwindowcapture_la_LIBADD = @COMPIZ_LIBS@
nodist_libwindowcapture_la_SOURCES = windowcapture_options.c windowcapture_options.h
dist_libwindowcapture_la_SOURCES = windowcapture.c

BUILT_SOURCES = $(nodist_libwindowcapture_la_SOURCES)

AM_CPPFLAGS =                              \
	-I$(top_srcdir)/include             \
	@COMPIZ_CFLAGS@                     \
	-DDATADIR='"$(compdatadir)"'        \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"'

moduledir = $(plugindir)

module_LTLIBRARIES = libwindowcapture.la

CLEANFILES = *_options.c *_options.h

windowcapture_options.h: ../../metadata/windowcapture.xml.in
		$(BCOP_BIN) --header $@ $<

windowcapture_options.c: ../../metadata/windowcapture.xml.in windowcapture_options.h
		$(BCOP_BIN) --source $@ $<

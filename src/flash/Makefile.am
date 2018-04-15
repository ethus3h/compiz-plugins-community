## Process this file with automake to produce Makefile.in
PFLAGS=-module -avoid-version -no-undefined

libflash_la_LDFLAGS = $(PFLAGS)
libflash_la_LIBADD = @COMPIZ_LIBS@
nodist_libflash_la_SOURCES = flash_options.c flash_options.h
dist_libflash_la_SOURCES = flash.c

BUILT_SOURCES = $(nodist_libflash_la_SOURCES)

AM_CPPFLAGS =                              \
	-I$(top_srcdir)/include             \
	@COMPIZ_CFLAGS@                     \
	-DDATADIR='"$(compdatadir)"'        \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"'

moduledir = $(plugindir)

module_LTLIBRARIES = libflash.la

CLEANFILES = *_options.c *_options.h

flash_options.h: ../../metadata/flash.xml.in
		$(BCOP_BIN) --header $@ $<

flash_options.c: ../../metadata/flash.xml.in flash_options.h
		$(BCOP_BIN) --source $@ $<

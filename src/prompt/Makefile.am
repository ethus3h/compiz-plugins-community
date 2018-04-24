## Process this file with automake to produce Makefile.in
PFLAGS=-module -avoid-version -no-undefined

libprompt_la_LDFLAGS = $(PFLAGS)
libprompt_la_LIBADD = @COMPIZ_LIBS@
nodist_libprompt_la_SOURCES = prompt_options.c prompt_options.h
dist_libprompt_la_SOURCES = prompt.c

BUILT_SOURCES = $(nodist_libprompt_la_SOURCES)

AM_CPPFLAGS =                              \
	-I$(top_srcdir)/include             \
	@COMPIZ_CFLAGS@                     \
	-DDATADIR='"$(compdatadir)"'        \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"'

moduledir = $(plugindir)

module_LTLIBRARIES = libprompt.la

CLEANFILES = *_options.c *_options.h

prompt_options.h: ../../metadata/prompt.xml.in
		$(BCOP_BIN) --header $@ $<

prompt_options.c: ../../metadata/prompt.xml.in prompt_options.h
		$(BCOP_BIN) --source $@ $<

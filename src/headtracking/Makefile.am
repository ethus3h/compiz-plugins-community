## Process this file with automake to produce Makefile.in
PFLAGS=-module -avoid-version -no-undefined

libheadtracking_la_LDFLAGS = $(PFLAGS)
libheadtracking_la_LIBADD = @COMPIZ_LIBS@
nodist_libheadtracking_la_SOURCES = headtracking_options.c headtracking_options.h
dist_libheadtracking_la_SOURCES = chipmunk.c chipmunk.h cpArbiter.c cpArbiter.h cpArray.c cpArray.h cpBB.c cpBB.h cpBody.c cpBody.h cpCollision.c cpCollision.h cpHashSet.c cpHashSet.h cpJoint.c cpJoint.h cpPolyShape.c cpPolyShape.h cpShape.c cpShape.h cpSpace.c cpSpace.h cpSpaceHash.c cpSpaceHash.h cpVect.c cpVect.h headtracking.c prime.h

BUILT_SOURCES = $(nodist_libheadtracking_la_SOURCES)

AM_CPPFLAGS =                              \
	-I$(top_srcdir)/include             \
	@COMPIZ_CFLAGS@                     \
	-DDATADIR='"$(compdatadir)"'        \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"'          \
	-std=c99

moduledir = $(plugindir)

module_LTLIBRARIES = libheadtracking.la

CLEANFILES = *_options.c *_options.h

headtracking_options.h: ../../metadata/headtracking.xml.in
		$(BCOP_BIN) --header $@ $<

headtracking_options.c: ../../metadata/headtracking.xml.in headtracking_options.h
		$(BCOP_BIN) --source $@ $<

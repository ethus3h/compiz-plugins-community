## Process this file with automake to produce Makefile.in
PFLAGS=-module -avoid-version -no-undefined -c99

libnewton_la_LDFLAGS = $(PFLAGS)
libnewton_la_LIBADD = @COMPIZ_LIBS@
nodist_libnewton_la_SOURCES = newton_options.c newton_options.h
dist_libnewton_la_SOURCES = chipmunk.c chipmunk.h cpArbiter.c cpArbiter.h cpArray.c cpArray.h cpBB.c cpBB.h cpBody.c cpBody.h cpCollision.c cpCollision.h cpHashSet.c cpHashSet.h cpJoint.c cpJoint.h cpPolyShape.c cpPolyShape.h cpShape.c cpShape.h cpSpace.c cpSpace.h cpSpaceHash.c cpSpaceHash.h cpVect.c cpVect.h newton.c prime.h

BUILT_SOURCES = $(nodist_libnewton_la_SOURCES)

AM_CPPFLAGS =                              \
	-I$(top_srcdir)/include             \
	@COMPIZ_CFLAGS@                     \
	-DDATADIR='"$(compdatadir)"'        \
	-DLIBDIR='"$(libdir)"'              \
	-DLOCALEDIR="\"@datadir@/locale\""  \
	-DIMAGEDIR='"$(imagedir)"'

moduledir = $(plugindir)

module_LTLIBRARIES = libnewton.la

CLEANFILES = *_options.c *_options.h

newton_options.h: ../../metadata/newton.xml.in
		$(BCOP_BIN) --header $@ $<

newton_options.c: ../../metadata/newton.xml.in newton_options.h
		$(BCOP_BIN) --source $@ $<

ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

SUBDIRS = metadata src po icons images data

EXTRA_DIST =		         \
	AUTHORS		         \
	LICENSE.md	         \
	NEWS		         \
	README.md	         \
	VERSION		         \
	autogen.sh	         \
        compiz-visualevent.pc.in \
	intltool-extract.in      \
	intltool-merge.in        \
	intltool-update.in

DISTCLEANFILES =		 \
	po/stamp-it		 \
	intltool-extract	 \
	intltool-merge		 \
	intltool-update		 \
	po/.intltool-merge-cache

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA =	\
	compiz-visualevent.pc
	
# Build ChangeLog from GIT  history
ChangeLog:
	$(AM_V_GEN) if test -d $(top_srcdir)/.git; then \
		GIT_DIR="$(top_srcdir)/.git" git log --stat > $@; \
	fi

dist: ChangeLog

.PHONY: ChangeLog

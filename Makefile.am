NULL =

SUBDIRS = po

if HAVE_GRECORD
SUBDIRS += grecord
endif

if HAVE_SOUND_THEME
SUBDIRS += sound-theme
endif

if HAVE_GSTMIXER
SUBDIRS += gst-mixer
endif

if HAVE_GSTPROPS
SUBDIRS += gstreamer-properties
endif

DISTCHECK_CONFIGURE_FLAGS = \
	--disable-scrollkeeper	\
	--enable-more-warnings  \
	--enable-compile-warnings=maximum

DIST_SUBDIRS = \
	po 			\
	grecord			\
	gst-mixer		\
	gstreamer-properties	\
	sound-theme		\
	$(NULL)

EXTRA_DIST = \
	.version		\
	autogen.sh		\
	configure		\
	COPYING			\
	COPYING-DOCS		\
	COPYING.grecord		\
	COPYING.gst-mixer	\
	MAINTAINERS		\
        build-aux/git-version-gen \
	m4/as-compiler-flag.m4	\
	m4/as-version.m4	\
	ChangeLog.pre-2-26	\
	$(INTLTOOL_BUILT:=.in)	\
	$(NULL)

DISTCLEANFILES = \
	po/.intltool-merge-cache	\
	$(NULL)

ACLOCAL_AMFLAGS = -I m4

# see build-aux/git-version-get for details
BUILT_SOURCES = $(top_srcdir)/.version
$(top_srcdir)/.version:
	echo $(VERSION) > $@-t && mv $@-t $@
dist-hook:
	echo $(VERSION) > $(distdir)/.tarball-version

-include $(top_srcdir)/git.mk

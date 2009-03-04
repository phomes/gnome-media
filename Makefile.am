NULL =

SUBDIRS = po

if HAVE_GMP
SUBDIRS += profiles
endif

if HAVE_CDDBSLAVE
SUBDIRS += cddb-slave2
endif

if HAVE_VUMETER
SUBDIRS += vu-meter
endif

if HAVE_GRECORD
SUBDIRS += grecord
endif

if HAVE_GNOMECD
SUBDIRS += gnome-cd
endif

if HAVE_SOUND_THEME
SUBDIRS += sound-theme
endif

if HAVE_PULSEAUDIO
SUBDIRS += gnome-volume-control
endif

if HAVE_GSTMIXER
SUBDIRS += gst-mixer
endif

if HAVE_GSTPROPS
SUBDIRS += gstreamer-properties
endif

INTLTOOL_BUILT = \
        intltool-extract \
        intltool-merge \
        intltool-update \
	$(NULL)

DISTCHECK_CONFIGURE_FLAGS = \
	--disable-scrollkeeper	\
	--disable-gnomecd	\
	--enable-cddbslave	\
	--enable-vumeter	\
	--enable-more-warnings  \
	--enable-compile-warnings=maximum

DIST_SUBDIRS = \
	po 			\
	profiles		\
	cddb-slave2		\
	vu-meter		\
	gnome-volume-control	\
	grecord			\
	gst-mixer		\
	gnome-cd		\
	gstreamer-properties	\
	$(NULL)

EXTRA_DIST = \
	autogen.sh		\
	configure		\
	COPYING			\
	COPYING-DOCS		\
	COPYING.grecord		\
	COPYING.gst-mixer	\
	COPYING.profiles	\
	COPYING.vu-meter	\
	MAINTAINERS		\
	m4/as-compiler-flag.m4	\
	m4/as-version.m4	\
	m4/esd.m4		\
	gnome-doc-utils.make	\
	$(INTLTOOL_BUILT:=.in)	\
	$(NULL)

DISTCLEANFILES = \
	po/.intltool-merge-cache	\
	gnome-doc-utils.make		\
	$(INTLTOOL_BUILT)		\
	$(NULL)

ACLOCAL_AMFLAGS = -I m4

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

if HAVE_GSTMIXER
SUBDIRS += gst-mixer
endif

if HAVE_GSTPROPS
SUBDIRS += gstreamer-properties
endif

INTLTOOL_BUILT = \
        intltool-extract \
        intltool-merge \
        intltool-update

DISTCHECK_CONFIGURE_FLAGS = --disable-scrollkeeper --enable-cddbslave	\
--enable-vumeter --enable-gnomecd --enable-compile-warnings=maximum

DIST_SUBDIRS = po profiles cddb-slave2 \
	       vu-meter grecord gst-mixer gnome-cd \
               gstreamer-properties

EXTRA_DIST = autogen.sh configure COPYING-DOCS COPYING.grecord		\
	MAINTAINERS m4/as-compiler-flag.m4 m4/as-version.m4 m4/esd.m4	\
	gnome-doc-utils.make $(INTLTOOL_BUILT:=.in)

DISTCLEANFILES = po/.intltool-merge-cache \
                 gnome-doc-utils.make $(INTLTOOL_BUILT)

ACLOCAL_AMFLAGS = -I m4

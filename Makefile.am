SUBDIRS = po \
	  @PROGRAMS_GMP@ \
	  @PROGRAMS_CDDBSLAVE@ \
	  @PROGRAMS_VUMETER@ \
	  @PROGRAMS_GSTMIXER@ \
	  @PROGRAMS_GNOME_CD@ \
	  @PROGRAMS_GRECORD@ \
	  @PROGRAMS_GSTPROPS@

DISTCHECK_CONFIGURE_FLAGS = --disable-scrollkeeper

DIST_SUBDIRS = po profiles cddb-slave2 \
	       vu-meter grecord gst-mixer gnome-cd \
               gstreamer-properties

INTLTOOL_BUILT = \
        intltool-extract \
        intltool-merge \
        intltool-update

EXTRA_DIST = autogen.sh configure COPYING-DOCS COPYING.grecord		\
	MAINTAINERS m4/as-compiler-flag.m4 m4/as-version.m4 m4/esd.m4	\
	gnome-doc-utils.make $(INTLTOOL_BUILT:=.in)

DISTCLEANFILES = po/.intltool-merge-cache \
                 gnome-doc-utils.make \
                 $(INTLTOOL_BUILT)

ACLOCAL_AMFLAGS = -I m4

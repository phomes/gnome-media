# If you want to build these components, please cd to their respective
# directories and type `make'.

always_built_SUBDIRS = cddb-slave2

SUBDIRS = intl po omf-install $(always_built_SUBDIRS) \
	  @PROGRAMS_VUMETER@ @PROGRAMS_GMIX@ @PROGRAMS_GNOME_CD@ @PROGRAMS_GRECORD@

DIST_SUBDIRS = intl po macros omf-install $(always_built_SUBDIRS) \
	       vu-meter gmix tcd grecord

EXTRA_DIST = gnome-media.spec.in configure acinclude.m4 COPYING-DOCS \
		sgmldocs.make xml-i18n-extract.in xml-i18n-merge.in \
		xml-i18n-update.in

dist-hook: gnome-media.spec
	cp gnome-media.spec $(distdir)

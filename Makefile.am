# If you want to build these components, please cd to their respective
# directories and type `make'.

always_built_SUBDIRS = po macros cddbslave

# always distribute extace even if we can't build it
if BUILD_EXTACE
extace = extace
endif

SUBDIRS = $(always_built_SUBDIRS) $(extace) \
	  @PROGRAMS_VUMETER@ @PROGRAMS_GMIX@ @PROGRAMS_TCD@

DIST_SUBDIRS = $(always_built_SUBDIRS) extace \
	       vu-meter gmix tcd

EXTRA_DIST = gnome-media.spec.in configure

dist-hook:
	cp gnome-media.spec $(distdir)

## to automatically rebuild aclocal.m4 if any of the macros in `macros/' change
@MAINT@include macros/macros.dep
@MAINT@macros/macros.dep: macros/Makefile.am
@MAINT@	cd macros && $(MAKE) macros.dep

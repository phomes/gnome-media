# If you want to build these components, please cd to their respective
# directories and type `make'.
if FALSE
gnomovision = gnomovision
else
gnomovision =
endif

SUBDIRS = po intl macros	\
	@PROGRAMS_GMIX@		\
	@PROGRAMS_TCD@		\
	@PROGRAMS_VUMETER@	\
	@PROGRAMS_EXTACE@	\
	cddbslave		\
	@PROGRAMS_GNOMOVISION@

EXTRA_DIST = gnome-media.spec configure

## to automatically rebuild aclocal.m4 if any of the macros in `macros/' change
@MAINT@include macros/macros.dep
@MAINT@macros/macros.dep: macros/Makefile.am
@MAINT@	cd macros && $(MAKE) macros.dep

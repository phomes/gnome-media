SUBDIRS = po intl macros	\
	@PROGRAMS_GMIX@		\
	@PROGRAMS_TCD@		\
	@PROGRAMS_VUMETER@	\
	cddbslave
#
# broken with current esound
#	@PROGRAMS_EXTACE@	

#
# broken too
# 	@PROGRAMS_GNOMOVISION

EXTRA_DIST = gnome-media.spec configure

## to automatically rebuild aclocal.m4 if any of the macros in `macros/' change
@MAINT@include macros/macros.dep
@MAINT@macros/macros.dep: macros/Makefile.am
@MAINT@	cd macros && $(MAKE) macros.dep

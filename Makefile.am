SUBDIRS = po intl macros	\
	@PROGRAMS_GMIX@		\
	@PROGRAMS_GNOMOVISION@  \
	@PROGRAMS_TCD@

## to automatically rebuild aclocal.m4 if any of the macros in `macros/' change
@MAINT@include macros/macros.dep
@MAINT@macros/macros.dep: macros/Makefile.am
@MAINT@	cd macros && $(MAKE) macros.dep

# Generated automatically from Makefile.in by configure.
# Makefile.in generated automatically by automake 1.2 from Makefile.am

# Copyright (C) 1994, 1995, 1996, 1997 Free Software Foundation, Inc.
# This Makefile.in is free software; the Free Software Foundation
# gives unlimited permission to copy, distribute and modify it.


SHELL = /bin/sh

srcdir = .
top_srcdir = ..
prefix = /usr/local
exec_prefix = ${prefix}

bindir = ${exec_prefix}/bin
sbindir = ${exec_prefix}/sbin
libexecdir = ${exec_prefix}/libexec
datadir = ${prefix}/share
sysconfdir = ${prefix}/etc
sharedstatedir = ${prefix}/com
localstatedir = ${prefix}/var
libdir = ${exec_prefix}/lib
infodir = ${prefix}/info
mandir = ${prefix}/man
includedir = ${prefix}/include
oldincludedir = /usr/include

pkgdatadir = $(datadir)/gnome-media
pkglibdir = $(libdir)/gnome-media
pkgincludedir = $(includedir)/gnome-media

top_builddir = ..

ACLOCAL = aclocal -I macros
AUTOCONF = autoconf
AUTOMAKE = automake
AUTOHEADER = autoheader

INSTALL = /usr/bin/install -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA = ${INSTALL} -m 644
INSTALL_SCRIPT = ${INSTALL_PROGRAM}
transform = s,x,x,

NORMAL_INSTALL = true
PRE_INSTALL = true
POST_INSTALL = true
NORMAL_UNINSTALL = true
PRE_UNINSTALL = true
POST_UNINSTALL = true
host_alias = i586-pc-linux-gnu
host_triplet = i586-pc-linux-gnu
CATALOGS =  ko.gmo de.gmo fr.gmo it.gmo
CATOBJEXT = .gmo
CC = gcc
CFLAGS = -g -O2
CPPFLAGS = 
DATADIRNAME = share
GENCAT = 
GMOFILES =  ko.gmo de.gmo fr.gmo it.gmo
GMSGFMT = /usr/local/bin/msgfmt
GNOMEUI_LIBS = -lgnomeui -lgnome -lgnomesupport -ldl -lgdk_imlib   -ljpeg  -lz -lSM -lICE -L/usr/local/lib -L/usr/X11R6/lib -lgtk -lgdk -lglib -lXext -lX11 -lm
GNOME_INCLUDEDIR = -I/usr/local/include -I/usr/local/include/libgnomesupport -I/usr/local/lib/glib/include -I/usr/local/include -I/usr/X11R6/include
GNOME_LIBDIR = -L/usr/local/lib
GNOME_LIBS = -lgnome -lgnomesupport -ldl
GTKXMHTML_LIBS = -lgtkxmhtml -lXpm -ljpeg  -lz -lSM -lICE -L/usr/local/lib -L/usr/X11R6/lib -lgtk -lgdk -lglib -lXext -lX11 -lm
GT_NO = 
GT_YES = #YES#
INCLUDE_LOCALE_H = #include <locale.h>
INSTOBJEXT = .mo
INTLDEPS = 
INTLLIBS = 
INTLOBJS = 
LD = /usr/local/lib/gcc-lib/i686-pc-linux-gnu/egcs-2.90.27/ld
LDFLAGS = 
LIBTOOL = $(SHELL) $(top_builddir)/libtool
LN_S = ln -s
MAINT = 
MAKEINFO = /home/timg/projects/gnome-media/missing makeinfo
MKINSTALLDIRS = ./mkinstalldirs
MSGFMT = /usr/local/bin/msgfmt
NM = /usr/bin/nm -B
PACKAGE = gnome-media
POFILES =  ko.po de.po fr.po it.po
POSUB = po
PROGRAMS_GMIX = gmix
PROGRAMS_GNOMOVISION = tcd
PROGRAMS_TCD = 
RANLIB = ranlib
USE_INCLUDED_LIBINTL = no
USE_NLS = yes
VERSION = 0.13
l = 

Audiodir = $(datadir)/apps/Audio

bin_PROGRAMS = gtcd tcd

gtcd_SOURCES = 			\
	src/gtcd.c 		\
	src/cddb.c 		\
	src/socket.c 		\
	src/cdrom.c 		\
	src/gtracked.c 		\
	src/gabout.c 		\
	src/gcddb.c 		\
	src/cddb.h 		\
	src/socket.h 		\
	src/gtracked.h 		\
	src/gabout.h 		\
	src/gcddb.h		\
	src/tooltips.h

tcd_SOURCES = 			\
	src/tcd.c 		\
	src/cddb.c 		\
	src/socket.c 		\
	src/cdrom.c 		\
	src/cddb.h 		\
	src/socket.h 		\
	src/cdrom.h		\
	src/tcd.h		\
	src/tracked.h		\
	src/tracked.c

gtcd_LDADD = $(GNOME_LIBDIR) $(GNOMEUI_LIBS) $(INTLLIBS)

tcd_LDADD = -lm -lncurses

CLEANFILES = *.o core *~ libcdrom.a

INCLUDES = -I$(includedir) $(GNOME_INCLUDEDIR)

Audio_data = gtcd.desktop
mkinstalldirs = $(SHELL) $(top_srcdir)/mkinstalldirs
CONFIG_HEADER = ../config.h
CONFIG_CLEAN_FILES = 
PROGRAMS =  $(bin_PROGRAMS)


DEFS = -DHAVE_CONFIG_H -I. -I$(srcdir) -I..
LIBS = 
gtcd_OBJECTS =  gtcd.o cddb.o socket.o cdrom.o gtracked.o gabout.o \
gcddb.o
gtcd_DEPENDENCIES = 
gtcd_LDFLAGS = 
tcd_OBJECTS =  tcd.o cddb.o socket.o cdrom.o tracked.o
tcd_DEPENDENCIES = 
tcd_LDFLAGS = 
COMPILE = $(CC) $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS)
LTCOMPILE = $(LIBTOOL) --mode=compile $(CC) $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS)
LINK = $(LIBTOOL) --mode=link $(CC) $(CFLAGS) $(LDFLAGS) -o $@
DIST_COMMON =  README AUTHORS Makefile.am Makefile.in NEWS TODO


DISTFILES = $(DIST_COMMON) $(SOURCES) $(HEADERS) $(TEXINFOS) $(EXTRA_DIST)

TAR = tar
GZIP = --best
DEP_FILES =  .deps/cddb.P .deps/cdrom.P .deps/gabout.P .deps/gcddb.P \
.deps/gtcd.P .deps/gtracked.P .deps/socket.P .deps/tcd.P \
.deps/tracked.P
SOURCES = $(gtcd_SOURCES) $(tcd_SOURCES)
OBJECTS = $(gtcd_OBJECTS) $(tcd_OBJECTS)

default: all

.SUFFIXES:
.SUFFIXES: .c .lo .o
$(srcdir)/Makefile.in:  Makefile.am $(top_srcdir)/configure.in $(ACLOCAL_M4) 
	cd $(top_srcdir) && $(AUTOMAKE) --gnu tcd/Makefile

Makefile: $(srcdir)/Makefile.in $(top_builddir)/config.status $(BUILT_SOURCES)
	cd $(top_builddir) \
	  && CONFIG_FILES=$(subdir)/$@ CONFIG_HEADERS= $(SHELL) ./config.status


mostlyclean-binPROGRAMS:

clean-binPROGRAMS:
	test -z "$(bin_PROGRAMS)" || rm -f $(bin_PROGRAMS)

distclean-binPROGRAMS:

maintainer-clean-binPROGRAMS:

install-binPROGRAMS: $(bin_PROGRAMS)
	@$(NORMAL_INSTALL)
	$(mkinstalldirs) $(bindir)
	@list='$(bin_PROGRAMS)'; for p in $$list; do \
	  if test -f $$p; then \
	    echo " $(LIBTOOL)  --mode=install $(INSTALL_PROGRAM) $$p $(bindir)/`echo $$p|sed '$(transform)'`"; \
	    $(LIBTOOL)  --mode=install $(INSTALL_PROGRAM) $$p $(bindir)/`echo $$p|sed '$(transform)'`; \
	  else :; fi; \
	done

uninstall-binPROGRAMS:
	$(NORMAL_UNINSTALL)
	list='$(bin_PROGRAMS)'; for p in $$list; do \
	  rm -f $(bindir)/`echo $$p|sed '$(transform)'`; \
	done

.c.o:
	$(COMPILE) -c $<

mostlyclean-compile:
	rm -f *.o core

clean-compile:

distclean-compile:
	rm -f *.tab.c

maintainer-clean-compile:

.c.lo:
	$(LIBTOOL) --mode=compile $(COMPILE) -c $<

mostlyclean-libtool:
	rm -f *.lo

clean-libtool:
	rm -rf .libs

distclean-libtool:

maintainer-clean-libtool:

gtcd: $(gtcd_OBJECTS) $(gtcd_DEPENDENCIES)
	@rm -f gtcd
	$(LINK) $(gtcd_LDFLAGS) $(gtcd_OBJECTS) $(gtcd_LDADD) $(LIBS)

tcd: $(tcd_OBJECTS) $(tcd_DEPENDENCIES)
	@rm -f tcd
	$(LINK) $(tcd_LDFLAGS) $(tcd_OBJECTS) $(tcd_LDADD) $(LIBS)

tags: TAGS

ID: $(HEADERS) $(SOURCES)
	here=`pwd` && cd $(srcdir) && mkid -f$$here/ID $(SOURCES) $(HEADERS)

TAGS:  $(HEADERS) $(SOURCES)  $(TAGS_DEPENDENCIES)
	tags=; \
	here=`pwd`; \
	test -z "$(ETAGS_ARGS)$(SOURCES)$(HEADERS)$$tags" \
	  || (cd $(srcdir) && etags $(ETAGS_ARGS) $$tags  $(SOURCES) $(HEADERS) -o $$here/TAGS)

mostlyclean-tags:

clean-tags:

distclean-tags:
	rm -f TAGS ID

maintainer-clean-tags:

distdir = $(top_builddir)/$(PACKAGE)-$(VERSION)/$(subdir)

subdir = tcd

distdir: $(DISTFILES)
	here=`cd $(top_builddir) && pwd`; \
	top_distdir=`cd $(top_distdir) && pwd`; \
	cd $(top_srcdir) \
	  && $(AUTOMAKE) --include-deps --build-dir=$$here --srcdir-name=$(top_srcdir) --output-dir=$$top_distdir --gnu tcd/Makefile
	@for file in $(DISTFILES); do \
	  d=$(srcdir); \
	  test -f $(distdir)/$$file \
	  || ln $$d/$$file $(distdir)/$$file 2> /dev/null \
	  || cp -p $$d/$$file $(distdir)/$$file; \
	done

MKDEP = gcc -M $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS)

DEPS_MAGIC := $(shell mkdir .deps > /dev/null 2>&1 || :)
-include .deps/.P
.deps/.P: $(BUILT_SOURCES)
	echo > $@

-include $(DEP_FILES)

mostlyclean-depend:

clean-depend:

distclean-depend:

maintainer-clean-depend:
	rm -rf .deps

.deps/%.P: %.c
	@echo "Computing dependencies for $<..."
	@o='o'; \
	test -n "$o" && o='$$o'; \
	$(MKDEP) $< >$@.tmp \
	  && sed "s,^\(.*\)\.o:,\1.$$o \1.l$$o $@:," < $@.tmp > $@ \
	  && rm -f $@.tmp
info:
dvi:
check: all
	$(MAKE)
installcheck:
install-exec: install-binPROGRAMS
	@$(NORMAL_INSTALL)

install-data: 
	@$(NORMAL_INSTALL)

install: install-exec install-data all
	@:

uninstall: uninstall-binPROGRAMS

all: Makefile $(PROGRAMS)

install-strip:
	$(MAKE) INSTALL_PROGRAM='$(INSTALL_PROGRAM) -s' INSTALL_SCRIPT='$(INSTALL_PROGRAM)' install
installdirs:
	$(mkinstalldirs)  $(bindir)


mostlyclean-generic:
	test -z "$(MOSTLYCLEANFILES)" || rm -f $(MOSTLYCLEANFILES)

clean-generic:
	test -z "$(CLEANFILES)" || rm -f $(CLEANFILES)

distclean-generic:
	rm -f Makefile $(DISTCLEANFILES)
	rm -f config.cache config.log stamp-h stamp-h[0-9]*
	test -z "$(CONFIG_CLEAN_FILES)" || rm -f $(CONFIG_CLEAN_FILES)

maintainer-clean-generic:
	test -z "$(MAINTAINERCLEANFILES)" || rm -f $(MAINTAINERCLEANFILES)
	test -z "$(BUILT_SOURCES)" || rm -f $(BUILT_SOURCES)
mostlyclean:  mostlyclean-binPROGRAMS mostlyclean-compile \
		mostlyclean-libtool mostlyclean-tags mostlyclean-depend \
		mostlyclean-generic

clean:  clean-binPROGRAMS clean-compile clean-libtool clean-tags \
		clean-depend clean-generic mostlyclean

distclean:  distclean-binPROGRAMS distclean-compile distclean-libtool \
		distclean-tags distclean-depend distclean-generic clean
	rm -f config.status
	rm -f libtool

maintainer-clean:  maintainer-clean-binPROGRAMS maintainer-clean-compile \
		maintainer-clean-libtool maintainer-clean-tags \
		maintainer-clean-depend maintainer-clean-generic \
		distclean
	@echo "This command is intended for maintainers to use;"
	@echo "it deletes files that may require special tools to rebuild."

.PHONY: default mostlyclean-binPROGRAMS distclean-binPROGRAMS \
clean-binPROGRAMS maintainer-clean-binPROGRAMS uninstall-binPROGRAMS \
install-binPROGRAMS mostlyclean-compile distclean-compile clean-compile \
maintainer-clean-compile mostlyclean-libtool distclean-libtool \
clean-libtool maintainer-clean-libtool tags mostlyclean-tags \
distclean-tags clean-tags maintainer-clean-tags distdir \
mostlyclean-depend distclean-depend clean-depend \
maintainer-clean-depend info dvi installcheck install-exec install-data \
install uninstall all installdirs mostlyclean-generic distclean-generic \
clean-generic maintainer-clean-generic clean mostlyclean distclean \
maintainer-clean


# Tell versions [3.59,3.63) of GNU make to not export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:

Audiodir = $(datadir)/gnome/apps/Multimedia
pixmapdir = $(datadir)/pixmaps

EXTRA_DIST = README TODO NEWS gmix.desktop.in gnome-mixer.png \
	device_labels_linux.sed device_labels_linux.h \
	device_labels_alsa.sed device_labels_alsa.h

SUBDIRS = doc

INCLUDES = \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	$(MEDIA_CFLAGS) 				\
	-DGNOME_ICONDIR=\""$(pixmapdir)"\"

bin_PROGRAMS = gmix

gmix_SOURCES =	\
	gmix.c gmix.h prefs.c prefs.h

gmix_LDADD = \
	$(MEDIA_LIBS) $(INTLLIBS) $(GMIX_LIBS)

#Audio_in_files = gmix.desktop.in
#Audio_DATA = $(Audio_in_files:.desktop.in=.desktop)
@INTLTOOL_SERVER_RULE@

pixmap_DATA = gnome-mixer.png

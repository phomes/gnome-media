Audiodir = $(datadir)/applications
pixmapdir = $(datadir)/pixmaps

EXTRA_DIST = README TODO NEWS gnome-volume-control.desktop.in gnome-mixer.png \
	device_labels_linux.sed device_labels_linux.h \
	device_labels_alsa.sed device_labels_alsa.h

INCLUDES = \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	$(MEDIA_CFLAGS) 				\
	-DGNOME_ICONDIR=\""$(pixmapdir)"\"

bin_PROGRAMS = gnome-volume-control

gnome_volume_control_SOURCES =	\
	gmix.c gmix.h prefs.c prefs.h

gnome_volume_control_LDADD = \
	$(MEDIA_LIBS) $(INTLLIBS) $(GMIX_LIBS)

@INTLTOOL_DESKTOP_RULE@

Audio_in_files = gnome-volume-control.desktop.in
Audio_DATA = $(Audio_in_files:.desktop.in=.desktop)


pixmap_DATA = gnome-mixer.png

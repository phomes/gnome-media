Audiodir = $(datadir)/gnome/apps/Multimedia
pixmapdir = $(datadir)/pixmaps

EXTRA_DIST = README TODO NEWS gmix.desktop gnome-mixer.png \
	device_labels_linux.sed device_labels_linux.h

SUBDIRS = doc

INCLUDES = \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	$(GNOME_INCLUDEDIR) 				\
	-DGNOME_ICONDIR=\""$(pixmapdir)"\"

bin_PROGRAMS = gmix

gmix_SOURCES =	\
	gmix.c

gmix_LDADD = \
	$(GNOME_LIBDIR) $(GNOMEUI_LIBS) $(INTLLIBS)

Audio_DATA = gmix.desktop

pixmap_DATA = gnome-mixer.png

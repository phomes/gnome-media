Audiodir = $(datadir)/gnome/apps/Multimedia

INCLUDES = \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	$(GNOME_INCLUDEDIR)

bin_PROGRAMS = vumeter

vumeter_SOURCES = \
	gtkled.c	\
	gtkled.h	\
	gtkledbar.c	\
	gtkledbar.h	\
	main.c

vumeter_LDADD = \
	$(GNOME_LIBDIR) $(GNOMEUI_LIBS) 

BITMAPS = gnome-vumeter.png

pixmapdir = $(datadir)/pixmaps

pixmap_DATA = $(BITMAPS)

EXTRA_DIST = $(BITMAPS) vumeter.desktop README TODO

Audio_DATA = vumeter.desktop

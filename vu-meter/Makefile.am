Audiodir = $(datadir)/apps/Audio

INCLUDES = \
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

EXTRA_DIST = vumeter.desktop README NEWS TODO

Audio_DATA = vumeter.desktop

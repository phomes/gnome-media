Audiodir = $(datadir)/apps/Audio

INCLUDES = \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	$(GNOME_INCLUDEDIR) -I$(topsrcdir)

bin_PROGRAMS = vumeter

vumeter_SOURCES = \
	gtkled.c	\
	gtkled.h	\
	gtkledbar.c	\
	gtkledbar.h	\
	main.c

vumeter_LDADD = \
	$(GNOME_LIBDIR) $(GNOMEUI_LIBS) 

EXTRA_DIST = vumeter.desktop README TODO

Audio_DATA = vumeter.desktop

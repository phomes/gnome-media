Audiodir = $(datadir)/apps/Audio

INCLUDES = \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	$(GNOME_INCLUDEDIR)

noinst_PROGRAMS = sine

sine_LDADD = -lm

bin_PROGRAMS = extace

extace_SOURCES =	\
	extace.c

extace_LDADD = \
	$(GNOME_LIBDIR) $(GNOMEUI_LIBS) $(INTLLIBS) -lgsl $(ESD_LIBS)

EXTRA_DIST = extace.desktop README NEWS TODO

Audio_DATA = extace.desktop

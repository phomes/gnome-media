Audiodir = $(datadir)/apps/Audio

#EXTRA_DIST = README TODO NEWS gmix.desktop

INCLUDES = \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-I$(includedir)

bin_PROGRAMS = gmix

gmix_SOURCES =	\
	gmix.c

gmix_LDADD = \
	$(GNOME_LIBDIR) $(GNOMEUI_LIBS) $(INTLLIBS)

Audio_DATA = gmix.desktop

Multimediadir = $(datadir)/gnome/apps/Multimedia

INCLUDES = \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	$(GNOME_INCLUDEDIR)

noinst_PROGRAMS = sine

sine_LDADD = -lm

bin_PROGRAMS = extace

extace_SOURCES = \
        logo.xpm \
        stars.c \
	stars.h \
	extace.c

extace_LDADD = \
	$(GNOME_LIBDIR) $(GNOMEUI_LIBS) $(INTLLIBS) -lgslfft -lgslerr $(ESD_LIBS)

EXTRA_DIST = extace.desktop README NEWS TODO

Multimedia_DATA = extace.desktop

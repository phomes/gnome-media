Audiodir = $(datadir)/gnome/apps/Multimedia
Pixmapdir = $(datadir)/pixmaps

EXTRA_DIST = README TODO NEWS gtcd.desktop gtcd.png

# Should make sure that tcd will work with all flavors of curses.
# Make sure it can support curses w/o color and old versions.
if HAS_CURSES
CURSES_TCD=tcd
endif

bin_PROGRAMS = gtcd $(CURSES_TCD)

SUBDIRS = icons

gtcd_SOURCES = 		\
	gtcd.c 		\
	cddb.c 		\
	linux-cdrom.c 	\
	gtracked.c 	\
	cddb.h 		\
	gtracked.h 	\
	prefs.c		\
	prefs.h		\
	led.h		\
	led.c		\
	led.xpm		\
	linux-cdrom.h	\
	callbacks.c	\
	callbacks.h	\
	popup.c		\
	keybindings.c	\
	keybindings.h	\
	gtcd_public.h	\
	cddb_status.c	\
	cddb_props.c	\
	cddb_props.h

tcd_SOURCES = 		\
	tcd.c 		\
	cddb.c 		\
	linux-cdrom.c 	\
	cddb.h 		\
	linux-cdrom.h	\
	tcd.h		\
	tracked.h	\
	tracked.c

gtcd_LDADD = 	$(GNOME_LIBDIR) \
		$(GNOMEUI_LIBS) \
		$(INTLLIBS)

tcd_LDADD = $(GNOME_LIBDIR) -lglib -lm $(CURSES_LIBS) $(INTLLIBS)

CLEANFILES = *~

INCLUDES = -I$(includedir) $(GNOME_INCLUDEDIR) $(CURSES_INCLUDEDIR) \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" $(USE_CD_CHANGER)	\
	-DGNOME_ICONDIR=\""$(Pixmapdir)"\"

Audio_DATA = gtcd.desktop
Pixmap_DATA = gtcd.png
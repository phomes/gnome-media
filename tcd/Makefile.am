Audiodir = $(datadir)/apps/Audio

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
	socket.c 	\
	linux-cdrom.c 	\
	gtracked.c 	\
	gabout.c 	\
	gcddb.c 	\
	cddb.h 		\
	socket.h 	\
	gtracked.h 	\
	gabout.h 	\
	gcddb.h		\
	tooltips.h	\
	properties.c	\
	led.h		\
	led.c

tcd_SOURCES = 		\
	tcd.c 		\
	cddb.c 		\
	socket.c 	\
	linux-cdrom.c 	\
	cddb.h 		\
	socket.h 	\
	cdrom.h		\
	tcd.h		\
	tracked.h	\
	tracked.c

gtcd_LDADD = 	$(GNOME_LIBDIR) \
		$(GNOMEUI_LIBS) \
		$(INTLLIBS)

tcd_LDADD = -lm $(CURSES_LIBS)

CLEANFILES = *.o core *~ libcdrom.a

INCLUDES = -I$(includedir) $(GNOME_INCLUDEDIR) $(CURSES_INCLUDEDIR)
Audio_DATA = gtcd.desktop

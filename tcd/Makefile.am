Audiodir = $(datadir)/apps/Audio

bin_PROGRAMS = gtcd tcd

SUBDIRS = icons

gtcd_SOURCES = 		\
	gtcd.c 		\
	cddb.c 		\
	socket.c 	\
	linux-cdrom.c 	\
	linux-cdrom.h	\
	gtracked.c 	\
	gabout.c 	\
	gcddb.c 	\
	gcddb.h		\
	cddb.h 		\
	socket.h 	\
	gtracked.h 	\
	gabout.h 	\
	tooltips.h	\
	properties.c	\
	properties.h

tcd_SOURCES = 		\
	tcd.c 		\
	cddb.c 		\
	cddb.h 		\
	socket.c 	\
	linux-cdrom.c 	\
	linux-cdrom.h	\
	socket.h 	\
	tcd.h		\
	tracked.c	\
	tracked.h

gtcd_LDADD = 	$(GNOME_LIBDIR) \
		$(GNOMEUI_LIBS) \
		$(INTLLIBS)

tcd_LDADD = -lm -lncurses

CLEANFILES = *.o core *~ libcdrom.a

INCLUDES = -I$(includedir) $(GNOME_INCLUDEDIR) -I/usr/include/ncurses

EXTRA_DIST = gtcd.desktop

Audio_DATA = gtcd.desktop

Audiodir = $(datadir)/apps/Audio

bin_PROGRAMS = gtcd tcd

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
	properties.c	

tcd_SOURCES = 		\
	tcd.c 		\
	cddb.c 		\
	socket.c 	\
	linux-cdrom.c 	\
	cddb.h 		\
	socket.h 	\
	tcd.h		\
	tracked.h	\
	tracked.c

gtcd_LDADD = 	$(GNOME_LIBDIR) \
		$(GNOMEUI_LIBS) \
		$(INTLLIBS)

tcd_LDADD = -lm -lncurses

CLEANFILES = *.o core *~ libcdrom.a

INCLUDES = -I$(includedir) $(GNOME_INCLUDEDIR) -I/usr/include/ncurses

Audio_DATA = gtcd.desktop

Audiodir = $(datadir)/apps/Audio

bin_PROGRAMS = gtcd tcd

gtcd_SOURCES = 			\
	src/gtcd.c 		\
	src/cddb.c 		\
	src/socket.c 		\
	src/cdrom.c 		\
	src/gtracked.c 		\
	src/gabout.c 		\
	src/gcddb.c 		\
	src/cddb.h 		\
	src/socket.h 		\
	src/gtracked.h 		\
	src/gabout.h 		\
	src/gcddb.h		\
	src/tooltips.h

tcd_SOURCES = 			\
	src/tcd.c 		\
	src/cddb.c 		\
	src/socket.c 		\
	src/cdrom.c 		\
	src/cddb.h 		\
	src/socket.h 		\
	src/cdrom.h		\
	src/tcd.h		\
	src/tracked.h		\
	src/tracked.c

gtcd_LDADD = $(GNOME_LIBDIR) $(GNOMEUI_LIBS) $(INTLLIBS)

tcd_LDADD = -lm -lncurses

CLEANFILES = *.o core *~ libcdrom.a

INCLUDES = -I$(includedir) $(GNOME_INCLUDEDIR)

Audio_data = gtcd.desktop

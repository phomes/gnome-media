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

gtcd_LDADD = $(GTK_LIBS) $(OTHER_LIBS)

tcd_LDADD = $(CURSES_LIBS) $(COMMON_LIBS)

CLEANFILES = *.o core *~ libcdrom.a

INCLUDES = $(GTK_CFLAGS) $(OPTIONS)

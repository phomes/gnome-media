## Process this file with automake to produce Makefile.in

INCLUDES = \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\" \
	-I$(top_srcdir)/intl \
	$(MEDIA_CFLAGS)		\
	-DGNOME_ICONDIR=\""$(datadir)/pixmaps"\"

bin_PROGRAMS = grecord

grecord_SOURCES = \
	main.c \
	gui.c gui.h \
	grec.c grec.h \
	preferences.c preferences.h \
	sound.c sound.h \
	prog.h

grecord_LDADD = $(MEDIA_LIBS) $(INTLLIBS)


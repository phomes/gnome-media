Mediadir = $(datadir)/gnome/apps/Media

INCLUDES = -I$(includedir)	\
             $(GNOME_INCLUDEDIR)

bin_PROGRAMS = tvset # tgtv preferences

tvset_SOURCES = \
	xwindows.c	\
	tvset.c		\
	tv-videodev.c	\
	channels.h	\
	preferences.c   \
	channels.c

#tgtv_SOURCES = \
#	tgtv.c \
#	gtv.c gtv.h

#preferences_SOURCES = \
#	preferences.c

LINK_FLAGS  = $(GNOMOVISION_LIBS) $(GNOME_LIBDIR) $(GNOMEUI_LIBS)

tvset_LDADD = $(LINK_FLAGS)

#tgtv_LDADD = $(LINK_FLAGS)

XPM_FILES = about.xpm audio.xpm capturer.xpm dummy.xpm quit.xpm tmp.xpm picture.xpm

EXTRA_DIST = README.tvset gnomovision.desktop

Media_DATA = gnomovision.desktop

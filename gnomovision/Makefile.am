Mediadir = $(datadir)/apps/Media

INCLUDES = -I$(includedir)

bin_PROGRAMS = tvset about preferences

tvset_SOURCES = \
	xwindows.c	\
	tv-videodev.c	\
	channels.h	\
	channels.c

preferences_SOURCES = \
	preferences.c

about_SOURCES = \
	about.c

LINK_FLAGS  = -lxf86dga $(GNOMEUI_LIBS)

tvset_LDADD = 

XPM_FILES = about.xpm audio.xpm capturer.xpm dummy.xpm quit.xpm tmp.xpm picture.xpm

EXTRA_DIST = README.tvset gnomovision.desktop

Media_DATA = gnomovision.desktop
Mediadir = $(datadir)/apps/Media

INCLUDES = -I$(includedir)	\
             $(GNOME_INCLUDEDIR)

bin_PROGRAMS = tvset # preferences

tvset_SOURCES = \
	xwindows.c	\
	tvset.c		\
	tv-videodev.c	\
	channels.h	\
	channels.c

# preferences_SOURCES = \
#	preferences.c

LINK_FLAGS  = -lXxf86dga $(GNOMEUI_LIBS) $(GNOME_LIBDIR)

tvset_LDADD = $(LINK_FLAGS)

# preferences_LDADD = $(LINK_FLAGS)

XPM_FILES = about.xpm audio.xpm capturer.xpm dummy.xpm quit.xpm tmp.xpm picture.xpm

EXTRA_DIST = README.tvset gnomovision.desktop

Media_DATA = gnomovision.desktop
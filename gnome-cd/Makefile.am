SUBDIRS = images
INCLUDES =					\
	-I$(top_srcdir)				\
	$(GNOME_INCLUDEDIR)			\
	-I$(top_builddir)			\
	-I$(top_srcdir)/cddb-slave2		\
	$(MEDIA_CFLAGS)				\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-DTHEME_DIR=\""$(datadir)/pixmaps/gnome-cd/themes"\"

Audiodir = $(datadir)/applications
Pixmapdir = $(datadir)/pixmaps
EXTRA_DIST = gnome-cd.png gnome-cd.desktop.in

bin_PROGRAMS = gnome-cd
gnome_cd_SOURCES =		\
	gnome-cd.c		\
	gnome-cd.h		\
	callbacks.c		\
	callbacks.h		\
	cddb.c			\
	cddb.h			\
	cdrom.c			\
	cdrom.h			\
	display.c		\
	display.h		\
	linux-cdrom.c		\
	linux-cdrom.h		\
	theme.c

gnome_cd_LDADD = $(MEDIA_LIBS)		\
		 $(top_builddir)/cddb-slave2/libcddb-slave2.la	\
		 $(INTLLIBS)

@INTLTOOL_DESKTOP_RULE@
Audio_in_files = gnome-cd.desktop.in
Audio_DATA = $(Audio_in_files:.desktop.in=.desktop)
Pixmap_DATA = gnome-cd.png

SUBDIRS = src

EXTRA_DIST = \
	gnome-sound-recorder.desktop.in README COPYING AUTHORS INSTALL \
	pixmaps/gnome_mixer.xpm \
	gnome-grecord.png

@INTLTOOL_DESKTOP_RULE@
Applicationsdir = $(datadir)/applications
Applications_in_files = gnome-sound-recorder.desktop.in 
Applications_DATA = $(Applications_in_files:.desktop.in=.desktop)


Icondir = $(datadir)/pixmaps
Icon_DATA = gnome-grecord.png




SUBDIRS = src

EXTRA_DIST = \
	Soundrecorder.desktop README COPYING AUTHORS INSTALL \
	pixmaps/gnome_mixer.xpm grecord.keys.in grecord.mime \
	grecord.spec gnome-grecord.png

Applicationsdir = $(datadir)/gnome/apps/Multimedia
Applications_DATA = Soundrecorder.desktop 

Mimedir = $(datadir)/mime-info
Mime_DATA = grecord.keys grecord.mime

Icondir = $(datadir)/pixmaps
Icon_DATA = gnome-grecord.png

grecord.keys: grecord.keys.in Makefile
	sed -e 's^\@icondir\@^$(appicondir)^g' < $(srcdir)/grecord.keys.in > grecord.keys.tmp \
	&& mv grecord.keys.tmp grecord.keys 

Audiodir = $(datadir)/applications
pixmapdir = $(datadir)/pixmaps

EXTRA_DIST = gnome-volume-control.desktop.in gnome-volume-control.png

INCLUDES = \
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	$(MEDIA_CFLAGS) 				\
	-DGNOME_ICONDIR=\""$(pixmapdir)"\"

bin_PROGRAMS = gnome-volume-control

BUILT_SOURCES = 			\
	volume-marshal.c		\
	volume-marshal.h

gnome_volume_control_SOURCES =		\
	debug.c				\
	debug.h				\
	gnome-channel.c			\
	gnome-channel.h			\
	gnome-channel-group.c		\
	gnome-channel-group.h		\
	gnome-channel-widget.c		\
	gnome-channel-widget.h		\
	gnome-mixer.c			\
	gnome-mixer.h			\
	gnome-volume-control.c		\
	$(BUILT_SOURCES)

gnome_volume_control_LDADD = \
	$(MEDIA_LIBS) $(INTLLIBS) $(GMIX_LIBS) -lasound

device_flags = -export_dynamic -avoid-version
devicesdir = $(libdir)/gnome-volume-control/devices

devices_LTLIBRARIES =			\
	libalsa.la			\
	liboss.la


libalsa_la_SOURCES = 			\
	alsa-device.c			\
	alsa-mixer.c			\
	alsa-mixer.h			\
	alsa-channel.c			\
	alsa-channel.h			\
libalsa_la_LDFLAGS = $(device_flags)
libalsa_la_LIBADD = $(MEDIA_LIBS) $(GMIX_LIBS) -lasound


liboss_la_SOURCES =			\
	oss-device.c			\
	oss-channel.c			\
	oss-channel.h			\
	oss-mixer.c			\
	oss-mixer.h
libalsa_la_LDFLAGS = $(device_flags)
libalsa_la_LIBADD = $(MEDIA_LIBS) $(GMIX_LIBS)



@INTLTOOL_DESKTOP_RULE@

Audio_in_files = gnome-volume-control.desktop.in
Audio_DATA = $(Audio_in_files:.desktop.in=.desktop)

pixmap_DATA = gnome-volume-control.png

volume-marshal.h: volume-marshal.list $(GLIB_GENMARSHAL)
	$(GLIB_GENMARSHAL) $< --header --prefix=volume_control_marshal > $@

volume-marshal.c: volume-marshal.list $(GLIB_GENMARSHAL)
	echo "#include \"volume-marshal.h\"" > $@ && $(GLIB_GENMARSHAL) $< --body --prefix=volume_control_marshal >> $@

EXTRA_DIST = \
        volume-marshal.list

CLEANFILES = $(BUILT_SOURCES)
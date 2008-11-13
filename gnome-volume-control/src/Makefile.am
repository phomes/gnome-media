NULL =

bin_PROGRAMS = 					\
	gnome-volume-control-applet		\
	gnome-volume-control			\
	$(NULL)

INCLUDES =					\
	$(WARN_CFLAGS)				\
	$(VOLUME_CONTROL_CFLAGS)		\
	$(PULSEAUDIO_CFLAGS)			\
	-DLOCALE_DIR=\""$(datadir)/locale"\"	\
	-DLIBEXECDIR=\"$(libexecdir)\"		\
	-DGLADEDIR=\""$(pkgdatadir)"\"		\
	-DSOUND_DATA_DIR="\"$(datadir)/sounds\"" \
	$(NULL)

gnome_volume_control_applet_LDADD =		\
	$(VOLUME_CONTROL_LIBS)			\
	$(PULSEAUDIO_LIBS)			\
	$(NULL)

gnome_volume_control_applet_SOURCES =		\
	gvc-mixer-stream.h			\
	gvc-mixer-stream.c			\
	gvc-mixer-sink.h			\
	gvc-mixer-sink.c			\
	gvc-mixer-source.h			\
	gvc-mixer-source.c			\
	gvc-mixer-sink-input.h			\
	gvc-mixer-sink-input.c			\
	gvc-mixer-event-role.h			\
	gvc-mixer-event-role.c			\
	gvc-mixer-control.h			\
	gvc-mixer-control.c			\
	gvc-channel-bar.h			\
	gvc-channel-bar.c			\
	gvc-applet.h				\
	gvc-applet.c				\
	applet-main.c				\
	$(NULL)

gnome_volume_control_LDADD =			\
	$(VOLUME_CONTROL_LIBS)			\
	$(PULSEAUDIO_LIBS)			\
	$(NULL)

gnome_volume_control_SOURCES =			\
	gvc-mixer-stream.h			\
	gvc-mixer-stream.c			\
	gvc-mixer-sink.h			\
	gvc-mixer-sink.c			\
	gvc-mixer-source.h			\
	gvc-mixer-source.c			\
	gvc-mixer-sink-input.h			\
	gvc-mixer-sink-input.c			\
	gvc-mixer-event-role.h			\
	gvc-mixer-event-role.c			\
	gvc-mixer-control.h			\
	gvc-mixer-control.c			\
	gvc-channel-bar.h			\
	gvc-channel-bar.c			\
	gvc-sound-theme-chooser.h		\
	gvc-sound-theme-chooser.c		\
	sound-theme-file-utils.h		\
	sound-theme-file-utils.c		\
	gvc-mixer-dialog.h			\
	gvc-mixer-dialog.c			\
	dialog-main.c				\
	$(NULL)

BUILT_SOURCES =				\
	$(NULL)

CLEANFILES =				\
	$(BUILT_SOURCES)		\
	$(NULL)

MAINTAINERCLEANFILES =                  \
        *~                              \
        Makefile.in

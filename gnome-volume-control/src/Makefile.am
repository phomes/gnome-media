NULL =

ccpanelsdir = $(PANELS_DIR)
ccpanels_LTLIBRARIES = libsound.la

bin_PROGRAMS =					\
	gnome-volume-control-applet		\
	$(NULL)

AM_CPPFLAGS =					\
	$(WARN_CFLAGS)				\
	-I$(top_srcdir)/sound-theme		\
	$(VOLUME_CONTROL_CFLAGS)		\
	$(VOLUME_PANEL_CFLAGS)			\
	$(DISABLE_DEPRECATED)			\
	$(PULSEAUDIO_CFLAGS)			\
	-DLOCALE_DIR=\""$(datadir)/locale"\"	\
	-DLIBEXECDIR=\"$(libexecdir)\"		\
	-DGLADEDIR=\""$(pkgdatadir)"\"		\
	-DICON_DATA_DIR="\"$(pkgdatadir)/icons\"" \
	$(NULL)

noinst_LTLIBRARIES = libgnomevolumecontrol.la
libgnomevolumecontrol_la_SOURCES =		\
	gvc-mixer-card.h			\
	gvc-mixer-card.c			\
	gvc-mixer-card-private.h		\
	gvc-mixer-stream.h			\
	gvc-mixer-stream.c			\
	gvc-mixer-stream-private.h		\
	gvc-channel-map.h			\
	gvc-channel-map.c			\
	gvc-channel-map-private.h		\
	gvc-mixer-sink.h			\
	gvc-mixer-sink.c			\
	gvc-mixer-source.h			\
	gvc-mixer-source.c			\
	gvc-mixer-sink-input.h			\
	gvc-mixer-sink-input.c			\
	gvc-mixer-source-output.h		\
	gvc-mixer-source-output.c		\
	gvc-mixer-event-role.h			\
	gvc-mixer-event-role.c			\
	gvc-mixer-control.h			\
	gvc-mixer-control.c			\
	gvc-mixer-control-private.h		\
	gvc-channel-bar.h			\
	gvc-channel-bar.c			\
	gvc-log.h				\
	gvc-log.c				\
	gvc-pulseaudio-fake.h			\
	$(NULL)

gnome_volume_control_applet_LDADD =		\
	-lm					\
	libgnomevolumecontrol.la		\
	$(VOLUME_CONTROL_LIBS)			\
	$(PULSEAUDIO_LIBS)			\
	$(NULL)

gnome_volume_control_applet_SOURCES =		\
	gvc-stream-status-icon.h		\
	gvc-stream-status-icon.c		\
	gvc-applet.h				\
	gvc-applet.c				\
	applet-main.c				\
	$(NULL)

libsound_la_LIBADD =					\
	-lm						\
	libgnomevolumecontrol.la			\
	$(top_builddir)/sound-theme/libsoundtheme.la	\
	$(VOLUME_PANEL_LIBS)				\
	$(PULSEAUDIO_LIBS)				\
	$(NULL)

libsound_la_LDFLAGS = -avoid-version -module -no-undefined

libsound_la_SOURCES =				\
	gvc-balance-bar.h			\
	gvc-balance-bar.c			\
	gvc-mixer-dialog.h			\
	gvc-mixer-dialog.c			\
	gvc-level-bar.h				\
	gvc-level-bar.c				\
	gvc-combo-box.h				\
	gvc-combo-box.c				\
	gvc-speaker-test.h			\
	gvc-speaker-test.c			\
	cc-sound-panel.c			\
	cc-sound-panel.h			\
	$(NULL)

BUILT_SOURCES =				\
	$(NULL)

CLEANFILES =				\
	$(BUILT_SOURCES)		\
	$(NULL)

MAINTAINERCLEANFILES =                  \
        *~                              \
        Makefile.in

-include $(top_srcdir)/git.mk

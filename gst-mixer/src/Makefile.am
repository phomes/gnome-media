
if HAVE_SOUND_THEME
GVC_ST_LIBS = $(SOUND_THEME_LIBS) \
	$(top_builddir)/sound-theme/libsoundtheme.la
endif

AM_CPPFLAGS = \
	$(GSTMIXER_CFLAGS)				\
	-I$(top_srcdir)/sound-theme			\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-DDATA_DIR=\""$(pkgdatadir)"\"			\
	-DPIX_DIR=\""$(pkgdatadir)/pixmaps"\"

bin_PROGRAMS = gnome-volume-control

gnome_volume_control_SOURCES = \
	button.c \
	element.c \
	main.c \
	preferences.c \
	track.c \
	volume.c \
	window.c \
	misc.c

noinst_HEADERS = \
	button.h \
	element.h \
	keys.h \
	preferences.h \
	track.h \
	volume.h \
	window.h \
	misc.h

gnome_volume_control_LDFLAGS = \
	$(GVC_ST_LIBS) \
	$(GSTMIXER_LIBS)


-include $(top_srcdir)/git.mk

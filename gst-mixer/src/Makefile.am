INCLUDES = \
	$(GSTMIXER_CFLAGS)				\
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
	window.c

noinst_HEADERS = \
	button.h \
	element.h \
	keys.h \
	preferences.h \
	stock.h \
	track.h \
	volume.h \
	window.h

gnome_volume_control_LDFLAGS = \
	$(GSTMIXER_LIBS)

INCLUDES = \
	$(GSTMIXER_CFLAGS)				\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-DDATA_DIR=\""$(pkgdatadir)"\"			\
	-DPIX_DIR=\""$(pkgdatadir)/pixmaps"\"

bin_PROGRAMS = gst-mixer

gst_mixer_SOURCES = \
	mixer.c

gst_mixer_LDFLAGS = \
	$(GSTMIXER_LIBS)

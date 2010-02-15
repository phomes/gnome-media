## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-DGNOMELOCALEDIR=\""$(prefix)/$(DATADIRNAME)/locale"\"	\
	-I$(top_srcdir)						\
	-I$(top_srcdir)/grecord/src/recent-files/		\
	$(GSR_CFLAGS)						\
	-DDATADIR=\""$(datadir)"\"				\
	-DGSR_DATADIR=\""$(datadir)/gnome-sound-recorder"\"	\
	-DGSR_UIDIR=\""$(datadir)/gnome-sound-recorder/ui/"\"

bin_PROGRAMS = gnome-sound-recorder

gnome_sound_recorder_SOURCES =	\
	gnome-recorder.c	\
	gsr-window.c		\
	gsr-window.h

uidir = $(datadir)/gnome-sound-recorder/ui
ui_DATA = ui.xml

EXTRA_DIST = $(ui_DATA)

gnome_sound_recorder_LDADD =					\
	-lm							\
	$(GSR_LIBS) -lgstinterfaces-@GST_MAJORMINOR@		\
	$(top_builddir)/profiles/libgnome-media-profiles.la

-include $(top_srcdir)/git.mk

## Process this file with automake to produce Makefile.in

SUBDIRS = gst

INCLUDES = \
	-DGNOMELOCALEDIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-I$(top_srcdir) \
	-I$(top_srcdir)/intl \
	$(GSR_CFLAGS)		\
	-DDATADIR=\""$(datadir)"\"		\
	-DGSR_DATADIR=\""$(datadir)/gnome-sound-recorder"\"	\
	-DGSR_UIDIR=\""$(datadir)/gnome-sound-recorder/ui/"\"

bin_PROGRAMS = gnome-sound-recorder

gnome_sound_recorder_SOURCES =	\
	gnome-recorder.c	\
	gsr-window.c		\
	gsr-window.h

uidir = $(datadir)/gnome-sound-recorder/ui
ui_DATA = ui.xml

gnome_sound_recorder_LDADD = \
	$(GSR_LIBS) $(INTLLIBS) \
	$(top_builddir)/profiles/libgnome-media-profiles.la \
	gst/libgstrecelements.la

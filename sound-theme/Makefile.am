NULL =

SUBDIRS = sounds

noinst_LIBRARIES = libsoundtheme.a

AM_CPPFLAGS =						\
        $(WARN_CFLAGS)					\
        $(SOUND_THEME_CFLAGS)				\
        -DSOUND_DATA_DIR="\"$(datadir)/sounds\""	\
        -DSOUND_SET_DIR="\"$(pkgdatadir)/sounds\""	\
        $(NULL)

libsoundtheme_a_SOURCES = \
	gvc-sound-theme-chooser.h		\
	gvc-sound-theme-chooser.c		\
	sound-theme-file-utils.h		\
	sound-theme-file-utils.c		\
	$(NULL)

BUILT_SOURCES =				\
	$(NULL)

EXTRA_DIST = gvc-sound-theme-editor.c gvc-sound-theme-editor.h

CLEANFILES =				\
	$(BUILT_SOURCES)		\
	$(NULL)

MAINTAINERCLEANFILES =                  \
        *~                              \
        Makefile.in

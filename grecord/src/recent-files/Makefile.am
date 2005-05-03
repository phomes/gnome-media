
INCLUDES = \
	$(GSR_CFLAGS) 		\
	$(DISABLE_DEPRECATED)				

EGG_FILES =				\
	egg-recent.h			\
        egg-recent-model.c		\
        egg-recent-model.h		\
        egg-recent-item.c		\
        egg-recent-item.h		\
        egg-recent-view.c		\
        egg-recent-view.h		\
        egg-recent-view-gtk.c		\
        egg-recent-view-gtk.h		\
        egg-recent-util.c		\
	egg-recent-util.h

noinst_LTLIBRARIES = librecent.la

librecent_la_SOURCES = $(EGG_FILES)

EGGDIR=$(srcdir)/../../../../libegg/libegg/recent-files
regenerate-built-sources:
	EGGFILES="$(EGG_FILES)" EGGDIR="$(EGGDIR)" $(srcdir)/update-from-egg.sh


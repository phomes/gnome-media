SUBDIRS = images
INCLUDES =					\
	-I$(top_srcdir)				\
	$(GNOME_INCLUDEDIR)			\
	-I$(includedir)				\
	-I$(top_builddir)			\
	$(MEDIA_CFLAGS)				\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"

bin_PROGRAMS = gnome-cd
gnome_cd_SOURCES =		\
	gnome-cd.c		\
	gnome-cd.h		\
	callbacks.c		\
	callbacks.h		\
	cdrom.c			\
	cdrom.h			\
	display.c		\
	display.h		\
	linux-cdrom.c		\
	linux-cdrom.h

gnome_cd_LDADD = $(MEDIA_LIBS)		\
		 $(top_builddir)/cddb-slave2/libcddb-slave2.la	\
		 $(INTLLIBS)

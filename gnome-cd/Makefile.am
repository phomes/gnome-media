SUBDIRS = images
INCLUDES =					\
	-I$(top_srcdir)				\
	$(GNOME_INCLUDEDIR)			\
	-I$(includedir)				\
	-I$(top_builddir)			\
	-I$(top_srcdir)/cddb-slave2		\
	$(MEDIA_CFLAGS)				\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"

bin_PROGRAMS = gnome-cd
gnome_cd_SOURCES =		\
	gnome-cd.c		\
	gnome-cd.h		\
	callbacks.c		\
	callbacks.h		\
	cddb.c			\
	cddb.h			\
	cdrom.c			\
	cdrom.h			\
	display.c		\
	display.h		\
	linux-cdrom.c		\
	linux-cdrom.h

gnome_cd_LDADD = $(MEDIA_LIBS)		\
		 $(top_builddir)/cddb-slave2/libcddb-slave2.la	\
		 $(INTLLIBS)

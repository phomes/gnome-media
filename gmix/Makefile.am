Audiodir = $(datadir)/apps/Audio

INCLUDES = 				\
	-I../.. 			\
	-I$(srcdir)/../..	 	\
	-I$(includedir)			\
	$(X_CFLAGS)

bin_PROGRAMS = gmix

gmix_SOURCES =	\
	gmix.c

gmix_LDADD = \
	$(top_builddir)/libgnome/libgnome.la		\
	$(top_builddir)/libgnomeui/libgnomeui.la 	\
        $(X_LIBS)

Audio_DATA = gmix.desktop

EXTRA_DIST = README TODO NEWS gmix.desktop

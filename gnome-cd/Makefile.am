SUBDIRS = images
INCLUDES =					\
	-I$(top_srcdir)				\
	$(GNOME_INCLUDEDIR)			\
	-I$(top_builddir)			\
	-I$(top_srcdir)/cddb-slave2		\
	$(MEDIA_CFLAGS)				\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-DTHEME_DIR=\""$(datadir)/pixmaps/gnome-cd/themes"\"

Audiodir = $(datadir)/applications
Pixmapdir = $(datadir)/pixmaps
EXTRA_DIST = solaris-cdrom.c		\
	     solaris-cdrom.h		\
	     linux-cdrom.c		\
	     linux-cdrom.h		\
	     gnome-cd.png gnome-cd.desktop.in gnome-cd.schemas.in

IDLS = ../cddb-slave2/GNOME_Media_CDDBSlave2.idl

IDL_GENERATED =				\
	GNOME_Media_CDDBSlave2.h	\
	GNOME_Media_CDDBSlave2-common.c	\
	GNOME_Media_CDDBSlave2-skels.c	\
	GNOME_Media_CDDBSlave2-stubs.c

GNOME_Media_CDDBSlave2-impl.o: GNOME_Media_CDDBSlave.h

$(IDL_GENERATED): $(IDLS)
	$(ORBIT_IDL) -I $(BONOBO_IDLDIR) -I $(BONOBO_ACT_IDLDIR)	\
	-I $(srcdir) -I $(datadir)/idl	$(IDLS)
	
bin_PROGRAMS = gnome-cd
gnome_cd_SOURCES =		\
	$(IDL_GENERATED)	\
	gnome-cd.c		\
	gnome-cd.h		\
	gnome-cd-type.h		\
	callbacks.c		\
	callbacks.h		\
	cddb.c			\
	cddb.h			\
	cdrom.c			\
	cdrom.h			\
	display.c		\
	display.h		\
	$(CDROM_HOST)-cdrom.c	\
	$(CDROM_HOST)-cdrom.h	\
	preferences.c		\
	preferences.h		\
	theme.c

gnome_cd_LDADD = $(MEDIA_LIBS)		\
		 $(top_builddir)/cddb-slave2/libcddb-slave2.la	\
		 $(INTLLIBS)

@INTLTOOL_DESKTOP_RULE@
Audio_in_files = gnome-cd.desktop.in
Audio_DATA = $(Audio_in_files:.desktop.in=.desktop)
Pixmap_DATA = gnome-cd.png

schemadir=$(sysconfdir)/gconf/schemas
schema_DATA=gnome-cd.schemas

install-schemas:
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(srcdir)/$(schema_DATA)

install-data-local: install-schemas

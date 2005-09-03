SUBDIRS = src doc

EXTRA_DIST = \
	README COPYING AUTHORS INSTALL \
	gnome-sound-recorder.desktop.in \
	pixmaps/gnome_mixer.xpm \
	gnome-sound-recorder.schemas.in \
	gnome-grecord.png

@INTLTOOL_DESKTOP_RULE@
@INTLTOOL_SCHEMAS_RULE@

Applicationsdir = $(datadir)/applications
Applications_in_files = gnome-sound-recorder.desktop.in 
Applications_DATA = $(Applications_in_files:.desktop.in=.desktop)

schemadir=$(GCONF_SCHEMA_FILE_DIR)
schema_DATA=gnome-sound-recorder.schemas

pixmapdir = $(datadir)/pixmaps
pixmap_DATA = gnome-grecord.png

CLEANFILES=$(Applications_DATA) $(schema_DATA)
install-schemas:
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(schema_DATA) || true

if GCONF_SCHEMAS_INSTALL
install-data-local: install-schemas
else
install-data-local:
endif


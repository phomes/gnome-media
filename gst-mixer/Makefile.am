Desktopdir = $(datadir)/applications
SUBDIRS = pixmaps src doc

EXTRA_DIST = $(Desktop_in_files) \
        gnome-volume-control.schemas.in

@INTLTOOL_DESKTOP_RULE@
@INTLTOOL_SCHEMAS_RULE@

Desktop_in_files = gnome-volume-control.desktop.in
Desktop_DATA = $(Desktop_in_files:.desktop.in=.desktop)
schemadir=$(GCONF_SCHEMA_FILE_DIR)
schema_DATA=gnome-volume-control.schemas

install-schemas:
	 GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(schema_DATA) || true

if GCONF_SCHEMAS_INSTALL
install-data-local: install-schemas
else
install-data-local:
endif

CLEANFILES = $(Desktop_DATA) $(schema_DATA)

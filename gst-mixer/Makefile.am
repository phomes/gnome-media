Desktopdir = $(datadir)/applications
SUBDIRS = doc icons pixmaps src

@INTLTOOL_DESKTOP_RULE@
@INTLTOOL_SCHEMAS_RULE@

Desktop_in_files = gnome-volume-control.desktop.in.in
Desktop_DATA = $(Desktop_in_files:.desktop.in.in=.desktop)
schemadir=$(GCONF_SCHEMA_FILE_DIR)
schema_DATA=gnome-volume-control.schemas

install-schemas:
	 GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(schema_DATA) || true

if GCONF_SCHEMAS_INSTALL
install-data-local: install-schemas
else
install-data-local:
endif

EXTRA_DIST = $(Desktop_in_files) \
	gnome-volume-control.schemas.in 


CLEANFILES = $(Desktop_DATA) $(schema_DATA)
DISTCLEANFILES = $(Desktop_in_files:.desktop.in.in=.desktop.in)

-include $(top_srcdir)/git.mk

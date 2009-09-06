SUBDIRS = src doc icons

EXTRA_DIST = \
	README AUTHORS INSTALL ChangeLog.pre-2-26 \
	gnome-sound-recorder.desktop.in.in \
	gnome-sound-recorder.schemas.in


Applicationsdir = $(datadir)/applications
Applications_in_files = gnome-sound-recorder.desktop.in.in
Applications_DATA = $(Applications_in_files:.desktop.in.in=.desktop)

@INTLTOOL_DESKTOP_RULE@

schemadir=$(GCONF_SCHEMA_FILE_DIR)
schema_DATA=gnome-sound-recorder.schemas

@INTLTOOL_SCHEMAS_RULE@

CLEANFILES=$(Applications_DATA) $(schema_DATA)
DISTCLEANFILES=$(Applications_in_files:.desktop.in.in=.desktop.in)
install-schemas:
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(schema_DATA) || true

if GCONF_SCHEMAS_INSTALL
install-data-local: install-schemas
else
install-data-local:
endif

-include $(top_srcdir)/git.mk

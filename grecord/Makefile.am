SUBDIRS = src doc

EXTRA_DIST = \
	README COPYING AUTHORS INSTALL \
	gnome-sound-recorder.desktop.in.in \
	gnome-sound-recorder.schemas.in \
	gnome-grecord.png


Applicationsdir = $(datadir)/applications
Applications_in_files = gnome-sound-recorder.desktop.in.in
Applications_DATA = $(Applications_in_files:.desktop.in.in=.desktop)

@INTLTOOL_DESKTOP_RULE@

schemadir=$(GCONF_SCHEMA_FILE_DIR)
schema_DATA=gnome-sound-recorder.schemas

@INTLTOOL_SCHEMAS_RULE@

icondir = $(datadir)/icons/hicolor/48x48/apps
icon_DATA = gnome-grecord.png

CLEANFILES=$(Applications_DATA) $(schema_DATA)
DISTCLEANFILES=$(Applications_in_files:.desktop.in.in=.desktop.in)
install-schemas:
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(schema_DATA) || true

if GCONF_SCHEMAS_INSTALL
install-data-local: install-schemas
else
install-data-local:
endif

gtk_update_icon_cache = gtk-update-icon-cache -f -t $(datadir)/icons/hicolor
install-data-hook:
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache."; \
		$(gtk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated.  After install, run this:"; \
		echo "***   $(gtk_update_icon_cache)"; \
	fi

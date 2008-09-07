Audiodir = $(datadir)/applications

icondir = $(datadir)/icons/hicolor/48x48/apps

AM_CPPFLAGS = \
	-DGNOMELOCALEDIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	$(VUMETER_CFLAGS)			\
	-DGNOME_ICONDIR=\""$(pixmapdir)"\"

bin_PROGRAMS = vumeter

vumeter_SOURCES = \
	gtkled.c	\
	gtkled.h	\
	gtkledbar.c	\
	gtkledbar.h	\
	main.c

vumeter_LDADD = \
	$(VUMETER_LIBS) $(INTLLIBS)

BITMAPS = gnome-vumeter.png gnome-reclevel.png

icon_DATA = $(BITMAPS)

EXTRA_DIST = $(BITMAPS) vumeter.desktop.in.in reclevel.desktop.in.in README TODO

@INTLTOOL_DESKTOP_RULE@
Audio_in_files = vumeter.desktop.in.in reclevel.desktop.in.in
Audio_DATA = $(Audio_in_files:.desktop.in.in=.desktop)

CLEANFILES = $(Audio_DATA)
DISTCLEANFILES = $(Audio_in_files:.desktop.in.in=.desktop.in)

gtk_update_icon_cache = gtk-update-icon-cache -f -t $(datadir)/icons/hicolor
install-data-hook:
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache."; \
		$(gtk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated.  After install, run this:"; \
		echo "***   $(gtk_update_icon_cache)"; \
	fi

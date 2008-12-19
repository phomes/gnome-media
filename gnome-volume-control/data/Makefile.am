NULL =

SUBDIRS =			\
	icons			\
	$(NULL)

@INTLTOOL_DESKTOP_RULE@
autostartdir = $(datadir)/gnome/autostart
autostart_in_files = gnome-volume-control-applet.desktop.in
autostart_DATA = $(autostart_in_files:.desktop.in=.desktop)

appsdir = $(datadir)/applications
apps_in_files = gnome-volume-control.desktop.in
apps_DATA = $(apps_in_files:.desktop.in=.desktop)

EXTRA_DIST =					\
	$(autostart_in_files)			\
	gnome-volume-control.desktop.in.in	\
	$(NULL)

CLEANFILES =					\
	$(NULL)

DISTCLEANFILES =				\
	gnome-volume-control-applet.desktop	\
	$(NULL)

### compile flags used by all objects
INCLUDES =					\
	-I$(top_srcdir)				\
	-I$(top_builddir)			\
	$(CDDBSLAVE_CFLAGS)			\
	-DGNOMELOCALEDIR=\""$(prefix)/$(DATADIRNAME)/locale"\"	\
	-DDATADIR=\""$(datadir)"\"		\
	-DG_LOG_DOMAIN=\"CDDBSlave2\"

### intermediate libtool library generated from IDL source
IDLS = GNOME_Media_CDDBSlave2.idl

IDL_GENERATED =					\
	GNOME_Media_CDDBSlave2.h		\
	GNOME_Media_CDDBSlave2-common.c		\
	GNOME_Media_CDDBSlave2-skels.c		\
	GNOME_Media_CDDBSlave2-stubs.c

$(IDL_GENERATED): $(IDLS)
	$(ORBIT_IDL) -I $(BONOBO_IDLDIR) -I $(BONOBO_ACT_IDLDIR)	\
	-I $(srcdir) -I $(datadir)/idl 	\
		$(srcdir)/GNOME_Media_CDDBSlave2.idl

idldir = $(datadir)/idl
idl_DATA = $(IDLS)

noinst_LTLIBRARIES = libgnomemedia-cddbslave2.la
libgnomemedia_cddbslave2_la_SOURCES = $(IDL_GENERATED)

### installed cddb slave library
lib_LTLIBRARIES = libcddb-slave2.la
# since automake builds dependencies AFTER objects, and the object
# needs a generated header, we add a hack to make sure the .h gets
# generated before objects are compiled
cddb-slave-client.lo: libgnomemedia-cddbslave2.la

libcddb_slave2includedir = $(includedir)/cddb-slave2
libcddb_slave2include_HEADERS = cddb-slave-client.h

libcddb_slave2_la_SOURCES = cddb-slave-client.c
libcddb_slave2_la_LIBADD = libgnomemedia-cddbslave2.la

# installed programs
bin_PROGRAMS = cddb-slave2-properties

cddb_slave2_properties_LDADD = $(CDDBSLAVE_LIBS)
cddb_slave2_properties_SOURCES = 		\
	cddb-slave-capplet.c			\
	gnet.h					\
	gnet-private.c				\
	gnet-private.h				\
	iochannel.c				\
	iochannel.h				\
	inetaddr.c				\
	inetaddr.h				\
	socks.c					\
	socks.h					\
	socks-private.c				\
	socks-private.h				\
	tcp.c					\
	tcp.h


### libexec programs
libexec_PROGRAMS = CDDBSlave2 cddb-track-editor

CDDBSlave2_SOURCES =				\
	cddb-slave.c				\
	cddb-slave.h				\
	cddb-parser.c				\
	cddb-parser.h				\
	gnet.h					\
	gnet-private.c				\
	gnet-private.h				\
	iochannel.c				\
	iochannel.h				\
	inetaddr.c				\
	inetaddr.h				\
	main.c					\
	socks.c					\
	socks.h					\
	socks-private.c				\
	socks-private.h				\
	tcp.c					\
	tcp.h

CDDBSlave2_LDADD =				\
	libgnomemedia-cddbslave2.la		\
	$(CDDBSLAVE_LIBS)			\
	$(INTLLIBS)

cddb_track_editor_SOURCES = 			\
	cddb-disclosure.c			\
	cddb-disclosure.h			\
	cddb-track-editor.c

cddb_track_editor_LDADD =			\
	$(CDDBSLAVE_LIBS)			\
	libcddb-slave2.la

### GConf schema files
schemadir   = $(GCONF_SCHEMA_FILE_DIR)
schema_in_files = CDDB-Slave2.schemas.in
schema_DATA = $(schema_in_files:.schemas.in=.schemas)

install-schemas: $(schema_DATA)
	GCONF_CONFIG_SOURCE=$(GCONF_SCHEMA_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(srcdir)/$(schema_DATA) || true

if GCONF_SCHEMAS_INSTALL
# make sure we try to install schemas on make install
install-data-local: install-schemas
else
install-data-local:
endif

### Bonobo server files
server_in_files = GNOME_Media_CDDBSlave2.server.in.in

serverdir = $(libdir)/bonobo/servers
server_DATA = $(server_in_files:.server.in.in=.server)
$(server_in_files:.server.in.in=.server.in): $(server_in_files)
	sed -e "s|\@LIBEXECDIR\@|$(libexecdir)|" $< > $@

### Intltool rules
@INTLTOOL_SERVER_RULE@
@INTLTOOL_DESKTOP_RULE@
@INTLTOOL_SCHEMAS_RULE@

### desktop files
desktopdir = $(datadir)/control-center-2.0/capplets
Desktop_in_files = cddb-slave.desktop.in
desktop_DATA = $(Desktop_in_files:.desktop.in=.desktop)

### pack up all of the extra files in the tarball
EXTRA_DIST =						\
	$(server_in_files)				\
	$(Desktop_in_files)				\
	$(IDLS)						\
	$(schema_in_files)

### clean up all of the generated stuff
CLEANFILES =						\
	$(IDL_GENERATED) $(desktop_DATA)		\
	$(server_in_files:.server.in.in=.server.in)	\
	$(server_in_files:.server.in.in=.server)	\
	$(schema_DATA)


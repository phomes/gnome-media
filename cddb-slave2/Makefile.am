INCLUDES =					\
	-I$(top_srcdir)				\
	-I$(top_builddir)			\
	-I$(includedir)				\
	$(CDDBSLAVE_CFLAGS)			\
	-DGNOMELOCALEDIR=\""$(datadir)/locale"\"	\
	-DG_LOG_DOMAIN=\"CDDBSlave2\"

bin_PROGRAMS = CDDBSlave2
lib_LTLIBRARIES = libcddb-slave2.la

IDLS = GNOME_Media_CDDBSlave2.idl

IDL_GENERATED =					\
	GNOME_Media_CDDBSlave2.h		\
	GNOME_Media_CDDBSlave2-common.c		\
	GNOME_Media_CDDBSlave2-skels.c		\
	GNOME_Media_CDDBSlave2-stubs.c

GNOME_Media_CDDBSlave2-impl.o: GNOME_Media_CDDBSlave.h

$(IDL_GENERATED): $(IDLS)
	$(ORBIT_IDL) -I $(srcdir) -I $(datadir)/idl 	\
		$(srcdir)/GNOME_Media_CDDBSlave2.idl

idldir = $(datadir)/idl
idl_DATA = $(IDLS)

CDDBSlave2_SOURCES =				\
	$(IDL_GENERATED)			\
	cddb-slave.c				\
	cddb-slave.h				\
	main.c

CDDBSlave2_LDADD =				\
	$(CDDBSLAVE_LIBS)				\
	$(INTLLIBS)

libcddb_slave2includedir = $(includedir)/cddb-slave2
libcddb_slave2_la_SOURCES =			\
	$(IDL_GENERATED)			\
	cddb-slave-client.c

libcddb_slave2include_HEADERS =			\
	cddb-slave-client.h

schemadir=$(sysconfdir)/gconf/schemas
schema_DATA=CDDB-Slave2.schemas

oafdir = $(DESTDIR)$(libdir)/bonobo/servers
oaf_in_files = GNOME_Media_CDDBSlave2.server.in
oaf_DATA = $(oaf_in_files:.server.in=.server)

@INTLTOOL_SERVER_RULE@

EXTRA_DIST = $(oaf_in_files) $(oaf_DATA) $(IDLS) $(schema_DATA)

install-schemas:
	GCONF_CONFIG_SOURCE=$(GCONF_CONFIG_SOURCE) $(GCONFTOOL) --makefile-install-rule $(srcdir)/$(schema_DATA)
 
install-data-local: install-schemas

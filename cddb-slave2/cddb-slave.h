/*
 * cddb-slave.h: Header for CDDBSlave object
 *
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Authors: Iain Holmes  <iain@ximian.com>
 */

#ifndef __CDDB_SLAVE_H__
#define __CDDB_SLAVE_H__

#include <bonobo/bonobo-xobject.h>
#include <bonobo/bonobo-event-source.h>
#include "GNOME_Media_CDDBSlave2.h"

#ifdef __cplusplus
extern "C" {
#pragma }
#endif /* __cplusplus */

#define CDDB_SLAVE_TYPE (cddb_slave_get_type ())
#define CDDB_SLAVE(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), CDDB_SLAVE_TYPE, CDDBSlave))
#define CDDB_SLAVE_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), CDDB_SLAVE_TYPE, CDDBSlaveClass))
#define IS_CDDB_SLAVE(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CDDB_SLAVE_TYPE))
#define IS_CDDB_SLAVE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), CDDB_SLAVE_TYPE))

typedef struct _CDDBSlave CDDBSlave;
typedef struct _CDDBSlavePrivate CDDBSlavePrivate;
typedef struct _CDDBSlaveClass CDDBSlaveClass;
typedef struct _CDDBEntry {
	char *discid;
	char *offsets;
	int ntrks, nsecs;
	
	GHashTable *fields;
} CDDBEntry;


struct _CDDBSlave {
	BonoboObject parent;

	CDDBSlavePrivate *priv;
};

struct _CDDBSlaveClass {
	BonoboObjectClass parent_class;

	POA_GNOME_Media_CDDBSlave2__epv epv;
};

GType cddb_slave_get_type ();
CDDBSlave *cddb_slave_new (const char *server,
			   int port);
CDDBSlave *cddb_slave_new_full (const char *server,
				int port,
				BonoboEventSource *event_source);
BonoboEventSource *cddb_slave_get_event_source (CDDBSlave *cddb);

#ifdef __cplusplus
}
#endif

#endif

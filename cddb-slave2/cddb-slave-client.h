/*
 * cddb-slave-client.c: Client side wrapper for accessing CDDBSlave really
 *                      easily.
 *
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Authors: Iain Holmes  <iain@ximian.com>
 */

#ifndef __CDDB_SLAVE_CLIENT_H__
#define __CDDB_SLAVE_CLIENT_H__

#include <bonobo/bonobo-object-client.h>

#ifdef __cplusplus
extern "C" {
#pragma }
#endif

#define CDDB_SLAVE_CLIENT_TYPE (cddb_slave_client_get_type ())
#define CDDB_SLAVE_CLIENT(obj) (GTK_CHECK_CAST ((obj), CDDB_SLAVE_CLIENT_TYPE, CDDBSlaveClient))
#define CDDB_SLAVE_CLIENT_CLASS(klass) (GTK_CHECK_CLASS_CAST ((klass), CDDB_SLAVE_CLIENT_TYPE, CDDBSlaveClientClass))
#define IS_CDDB_SLAVE_CLIENT(obj) (GTK_CHECK_TYPE ((obj), CDDB_SLAVE_CLIENT_TYPE))
#define IS_CDDB_SLAVE_CLIENT_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), CDDB_SLAVE_CLIENT_TYPE))

#define CDDB_SLAVE_CLIENT_CDDB_FINISHED "GNOME_Media_CDDBSlave2:CDDB-Finished"

typedef struct _CDDBSlaveClient CDDBSlaveClient;
typedef struct _CDDBSlaveClientPrivate CDDBSlaveClientPrivate;
typedef struct _CDDBSlaveClientClass CDDBSlaveClientClass;

struct _CDDBSlaveClient {
	BonoboObjectClient parent;

	CDDBSlaveClientPrivate *priv;
};

struct _CDDBSlaveClientClass {
	BonoboObjectClientClass parent_class;
};

GtkType cddb_slave_client_get_type (void);
void cddb_slave_client_construct (CDDBSlaveClient *client,
				  CORBA_Object corba_object);
CDDBSlaveClient *cddb_slave_client_new_from_id (const char *id);
CDDBSlaveClient *cddb_slave_client_new (void);
gboolean cddb_slave_client_query (CDDBSlaveClient *client,
				  const char *discid,
				  int ntrks,
				  const char *offsets,
				  int nsecs,
				  const char *name,
				  const char *version);

#ifdef __cplusplus
}
#endif

#endif

/*
 * cddb-slave-client.c: Client side wrapper for accessing CDDBSlave really
 *                      easily.
 *
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Authors: Iain Holmes  <iain@ximian.com>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <bonobo-activation/bonobo-activation.h>

#include "GNOME_Media_CDDBSlave2.h"
#include "cddb-slave-client.h"

#include <bonobo/bonobo-listener.h>
#include <bonobo/bonobo-object.h>
#include <bonobo/bonobo-exception.h>

#define CDDB_SLAVE_IID "OAFIID:GNOME_Media_CDDBSlave2"

#define PARENT_TYPE G_TYPE_OBJECT
static GObjectClass *parent_class = NULL;

struct _CDDBSlaveClientPrivate {
	CORBA_Object objref;
};

static void
finalize (GObject *object)
{
	CDDBSlaveClient *client;

	client = CDDB_SLAVE_CLIENT (object);
	if (client->priv == NULL)
		return;

	g_free (client->priv);
	client->priv = NULL;

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
class_init (CDDBSlaveClientClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = finalize;

	parent_class = g_type_class_peek_parent (klass);
}

static void
init (CDDBSlaveClient *client)
{
	client->priv = g_new (CDDBSlaveClientPrivate, 1);
}

/* Standard functions */
GType
cddb_slave_client_get_type (void)
{
	static GType client_type = 0;

	if (!client_type) {
		GTypeInfo client_info = {
			sizeof (CDDBSlaveClientClass),
			NULL, NULL, (GClassInitFunc) class_init, NULL, NULL,
			sizeof (CDDBSlaveClient), 0,
			(GtkObjectInitFunc) init,
		};

		client_type = g_type_register_static (PARENT_TYPE, "CDDBSlaveClient", &client_info, 0);
	}

	return client_type;
}

/*** Public API ***/
/**
 * cddb_slave_client_construct:
 * @client: The CDDBSlaveClient to construct.
 * @corba_object: The CORBA_Object to construct it from.
 *
 * Constructs @client from @corba_object.
 */
void 
cddb_slave_client_construct (CDDBSlaveClient *client,
			     CORBA_Object corba_object)
{
	g_return_if_fail (client != NULL);
	g_return_if_fail (IS_CDDB_SLAVE_CLIENT (client));
	g_return_if_fail (corba_object != CORBA_OBJECT_NIL);

#warning ASK MICHAEL MEEKS ABOUT THIS
	client->priv->objref = corba_object;
}

/**
 * cddb_slave_client_new_from_id:
 * @id: The oafiid of the component to make a client for.
 *
 * Makes a client for the object returned by activating @id.
 *
 * Returns: A newly created CDDBSlaveClient or NULL on error.
 */
CDDBSlaveClient *
cddb_slave_client_new_from_id (const char *id)
{
	CDDBSlaveClient *client;
	CORBA_Environment ev;
	CORBA_Object objref;
	
	g_return_val_if_fail (id != NULL, NULL);

	CORBA_exception_init (&ev);
	objref = bonobo_activation_activate_from_id ((char *) id, 0, NULL, &ev);
	if (BONOBO_EX (&ev)) {
		g_warning ("Could no activate %s.\n%s", id,
			   CORBA_exception_id (&ev));
		CORBA_exception_free (&ev);
		return NULL;
	}
	
	CORBA_exception_free (&ev);
	if (objref == CORBA_OBJECT_NIL) {
		g_warning ("Could not start component %s.", id);
		return NULL;
	}

	client = g_object_new (cddb_slave_client_get_type (), NULL);
	cddb_slave_client_construct (client, objref);

	return client;
}

/**
 * cddb_slave_client_new:
 * 
 * Creates a new CDDBSlaveClient, using the default CDDBSlave component.
 *
 * Returns: A newly created CDDBSlaveClient or NULL on error.
 */
CDDBSlaveClient *
cddb_slave_client_new (void)
{
	return cddb_slave_client_new_from_id (CDDB_SLAVE_IID);
}

/**
 * cddb_slave_client_query:
 * @client: The CDDBSlaveClient to perform the query.
 * @discid: The ID of the CD to be searched for.
 * @ntrks: The number of tracks on the CD.
 * @offsets: A string of all the frame offsets to the starting location of
 *           each track, seperated by spaces.
 * @nsecs: Total playing length of the CD in seconds.
 * @name: The name of the the program performing the query. eg. GTCD.
 * @version: The version of the program performing the query.
 *
 * Asks the CDDBSlave that @client is a client for to perform a query on the
 * CDDB server that it is set up to connect to.
 * The @name string will be sent as "@name (CDDBSlave 2)", 
 * eg. "GTCD (CDDBSlave 2)"
 *
 * Returns: A boolean indicating if there was an error in sending the query to
 *          the CDDBSlave
 */
gboolean
cddb_slave_client_query (CDDBSlaveClient *client,
			 const char *discid,
			 int ntrks,
			 const char *offsets,
			 int nsecs,
			 const char *name,
			 const char *version)
{
	GNOME_Media_CDDBSlave2 cddb;
	CORBA_Environment ev;
	gboolean result;

	g_return_val_if_fail (client != NULL, FALSE);
	g_return_val_if_fail (IS_CDDB_SLAVE_CLIENT (client), FALSE);
	g_return_val_if_fail (discid != NULL, FALSE);
	g_return_val_if_fail (ntrks > 0, FALSE);
	g_return_val_if_fail (offsets != NULL, FALSE);
	g_return_val_if_fail (nsecs > 0, FALSE);
	g_return_val_if_fail (name != NULL, FALSE);
	g_return_val_if_fail (version != NULL, FALSE);

	CORBA_exception_init (&ev);
	cddb = client->priv->objref;
	GNOME_Media_CDDBSlave2_query (cddb, discid, ntrks, offsets, nsecs,
				      name, version, &ev);
	if (ev._major != CORBA_NO_EXCEPTION) {
		g_warning ("Error sending request: %s", CORBA_exception_id (&ev));
		result = FALSE;
	} else {
		result = TRUE;
	}

	CORBA_exception_free (&ev);
	return result;
}

/**
 * cddb_slave_client_add_listener:
 * @client: Client of the CDDBSlave to add a listener to.
 * @listener: BonoboListener to add.
 *
 * Adds a listener to the CDDBSlave that belongs to @client.
 */
void
cddb_slave_client_add_listener (CDDBSlaveClient *client,
				BonoboListener *listener)
{
	CORBA_Object client_objref, listener_objref;
	CORBA_Object event_source;
	CORBA_Environment ev;

	g_return_if_fail (client != NULL);
	g_return_if_fail (IS_CDDB_SLAVE_CLIENT (client));
	g_return_if_fail (listener != NULL);
	g_return_if_fail (BONOBO_IS_LISTENER (listener));
	
	client_objref = client->priv->objref;
	listener_objref = bonobo_object_corba_objref (BONOBO_OBJECT (listener));
	
	CORBA_exception_init (&ev);
	event_source = Bonobo_Unknown_queryInterface (client_objref,
						      "IDL:Bonobo/EventSource:1.0", &ev);
	if (BONOBO_EX (&ev)) {
		g_warning ("Error doing QI for event source\n%s",
			   CORBA_exception_id (&ev));
		CORBA_exception_free (&ev);
		return;
	}

	/* Add the listener */
	Bonobo_EventSource_addListener (event_source, listener_objref, &ev);
	if (BONOBO_EX (&ev)) {
		g_warning ("Error adding listener\n%s", CORBA_exception_id (&ev));
		CORBA_exception_free (&ev);
		return;
	}

	CORBA_exception_free (&ev);
	return;
}

/**
 * cddb_slave_client_remove_listener:
 * @client: Client of the CDDBSlave to remove a listener from.
 * @listenerID: ID of the listener to remove.
 *
 * Removes a listener from the CDDBSlave that belongs to @client.
 */
int
cddb_slave_client_remove_listener (CDDBSlaveClient *client,
				   BonoboListener *listener)
{
	CORBA_Object client_objref;
	CORBA_Object event_source;
	CORBA_Object listener_objref;
	CORBA_Environment ev;

	g_return_if_fail (client != NULL);
	g_return_if_fail (IS_CDDB_SLAVE_CLIENT (client));
	g_return_if_fail (BONOBO_IS_LISTENER (listener));
	
	client_objref = client->priv->objref;
	listener_objref = bonobo_object_corba_objref (BONOBO_OBJECT (listener));
	
	CORBA_exception_init (&ev);
	event_source = Bonobo_Unknown_queryInterface (client_objref,
						      "IDL:Bonobo/EventSource:1.0", &ev);
	if (BONOBO_EX (&ev)) {
		g_warning ("Error doing QI for event source\n%s",
			   CORBA_exception_id (&ev));
		CORBA_exception_free (&ev);
		return;
	}

	/* Remove the listener */
	Bonobo_EventSource_removeListener (event_source, listener_objref, &ev);
	if (BONOBO_EX (&ev)) {
		g_warning ("Error removing listener\n%s", CORBA_exception_id (&ev));
	}
	
	CORBA_exception_free (&ev);
	return;
}


	

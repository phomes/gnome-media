/*
 * CDDBSlave 2
 *
 * Copyright (C) 2001 Ximian, Inc.
 *
 * Authors: Iain Holmes  <iain@ximian.com>
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libgnome/libgnome.h>
#include <bonobo-activation/bonobo-activation.h>
#include <libbonobo.h>

#include <libgnomevfs/gnome-vfs-init.h>
#include <gconf/gconf-client.h>

#include "cddb-slave.h"

#define CDDBSLAVE_IID "OAFIID:GNOME_Media_CDDBSlave2_Factory"
#define CDDB_SERVER "freedb.freedb.org"
#define CDDB_PORT 888;

static int running_objects = 0;

static void
cddb_destroy_cb (GObject *cddb,
		 gpointer data)
{
	running_objects--;

	if (running_objects <= 0) {
		bonobo_main_quit ();
	}
}

static BonoboObject *
factory_fn (BonoboGenericFactory *factory,
	    const char *component_id,
	    void *closure)
{
	GConfClient *client;
	CDDBSlave *cddb;
	gboolean auth;
	char *server;
	int port;

	/* Get GConf db */
	client = gconf_client_get_default ();
	if (client == NULL) {
		server = g_strdup ("freedb.freedb.org");
		port = 888;
	} else {
		
		/* Get server info */
		server = gconf_client_get_string (client, 
						  "/apps/CDDB-Slave2/server", NULL);
		port = gconf_client_get_int (client, "/apps/CDDB-Slave2/port", NULL);

		g_object_unref (G_OBJECT (client));
	}

	/* Create the new slave */
	cddb = cddb_slave_new (server, port);

	if (cddb == NULL) {
		g_error ("Could not create CDDB slave");
		return NULL;
	}

	/* Keep track of our objects */
	running_objects++;
	g_signal_connect (G_OBJECT (cddb), "destroy",
			  G_CALLBACK (cddb_destroy_cb), NULL);

	return BONOBO_OBJECT (cddb);
}
static gboolean
cddbslave_init (gpointer data)
{
	BonoboGenericFactory *factory;

	factory = bonobo_generic_factory_new (CDDBSLAVE_IID, factory_fn, NULL);
	if (factory == NULL) {
		g_error ("Cannot create factory");
		exit (1);
	}

	bonobo_running_context_auto_exit_unref (BONOBO_OBJECT (factory));

	return FALSE;
}

int 
main (int argc,
      char **argv)
{
	CORBA_ORB orb;

	bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
	textdomain (GETTEXT_PACKAGE);

#if 0
	/* We don't really want an X connection
	   This is a GUIless program */
	gtk_type_init ();
	gtk_signal_init ();

	gnomelib_init ("CDDBSlave-2", VERSION);
#endif
/*  	gnome_program_init ("CDDBSlave-2", VERSION, &libgnome_module_info, */
/*  			    argc, argv, NULL); */
	bonobo_init (&argc, argv);

	g_idle_add (cddbslave_init, NULL);
	bonobo_main ();

	exit (0);
}
	

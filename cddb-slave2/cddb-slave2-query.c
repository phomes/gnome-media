/*
 * cddb-slave2-query.c: perform a CDDB query using the CDDB-Slave2 component
 *
 * Copyright (C) <2004> Thomas Vander Stichele <thomas at apestaart dot org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdlib.h>                           /* strtol */

#include <glib.h>
#include <bonobo/bonobo-main.h>

#include <cddb-slave-client.h>
#include <GNOME_Media_CDDBSlave2.h>

static gboolean have_result = FALSE; /* set to TRUE from the event callback */
static gboolean query_ok = FALSE; /* set to TRUE from cb if query went ok */

static void
listener_event_cb (BonoboListener *listener,
                   const char *name,
                   const BonoboArg *arg,
                   CORBA_Environment *ev,
                   gpointer *user_data)
{
        GNOME_Media_CDDBSlave2_QueryResult *qr;

        qr = arg->_value;

        switch (qr->result) {
        case GNOME_Media_CDDBSlave2_OK:
                break;
	default:
		g_warning ("Lookup through slave failed");
	}
	query_ok = TRUE;
	have_result = TRUE;
}

static void
display_result (CDDBSlaveClient *slave, const char *discid)
{
	gchar *artist = NULL;
	gchar *title = NULL;
	gint ntracks;
	gint i;
	CDDBSlaveClientTrackInfo **track_info = NULL;

	artist = cddb_slave_client_get_artist (slave, discid);
	title = cddb_slave_client_get_disc_title (slave, discid);
	track_info = cddb_slave_client_get_tracks (slave, discid);
	ntracks = cddb_slave_client_get_ntrks (slave, discid);
	track_info = cddb_slave_client_get_tracks (slave, discid);

	g_print ("Results of CDDBSlave2 lookup:\n");
	g_print ("Artist:     %s\n", artist);
	g_print ("Disc Title: %s\n", title);
	g_print ("Tracks:     %d\n", ntracks);
	g_print ("\n");

	for (i = 1; i <= ntracks; ++i)
	{
		gint length;

		length = track_info[i - 1]->length;
		g_print ("Track %2d:   %s (%d:%d)\n", i,
                         track_info[i - 1]->name,
                         length / 60, length % 60);
	}
	g_print ("\n");
}

int
main (int argc,
      char *argv[])
{
	CDDBSlaveClient *slave = NULL;
	BonoboListener *listener = NULL;
	gchar *discid = NULL;
	char *endptr = NULL;
	gchar *offsets = NULL;
	gchar *string = NULL;
	gint ntracks;
	gint nsecs;
	gint i;

	/* initialize bonobo so we can use cddb_slave_client_new */
	/* watch out ! bonobo_init takes **argv, not ***argv like others */
	bonobo_init (&argc, argv);

	if (argc < 2)
	{
		g_print ("Please specify a disc id to look up.\n");
		return 1;
	}
	discid = g_strdup (argv[1]);

	if (argc < 3)
	{
		g_print ("Please specify the number of tracks.\n");
		return 2;
	}
	ntracks = strtol (argv[2], &endptr, 0);
	if (*endptr != '\0')
	{
		g_print ("Please specify an integer as the number of tracks.\n");
		return 3;
	}

	if (argc < 3 + ntracks)
	{
		g_print ("Please specify as many frame start offsets as tracks.\n");
		return 4;
	}

	/* take the first offset */
	offsets = g_strdup (argv[3]);

	/* and now add the others */
	for (i = 1; i < ntracks; ++i)
	{
		string = g_strdup_printf ("%s %s", offsets, argv[3 + i]);
		g_free (offsets);
		offsets = string;
	}
	if (argc < 3 + ntracks + 1)
	{
		g_print ("Please specify the total length in seconds.\n");
		return 5;
	}
	nsecs = strtol (argv[3 + ntracks], &endptr, 0);
	if (*endptr != '\0')
	{
		g_print ("Please specify an integer as the total length in seconds.\n");
		return 6;
	}


	/* output parsed info */
	g_print ("Looking up CDDBSlave2 entry with:\n");
	g_print ("discid:           %s\n", discid);
	g_print ("number of tracks: %d\n", ntracks);
	g_print ("offsets:          %s\n", offsets);
	g_print ("total seconds:    %d\n", nsecs);
	g_print ("\n");

	/* create a cddb slave client */
	slave = cddb_slave_client_new ();

	/* create a bonobo listener */
	listener = bonobo_listener_new (NULL, NULL);

	/* connect a callback to the listener */
	g_signal_connect (G_OBJECT (listener), "event-notify",
			  G_CALLBACK (listener_event_cb), NULL);

	/* add the listener to the slave */
	cddb_slave_client_add_listener (slave, listener);

	/* send the query */
	if (! cddb_slave_client_query (slave, discid, ntracks, offsets, nsecs,
                                       "cddb-slave2-query", "0"))
		g_warning ("Could not query");

	/* now wait for the result */
	while (!have_result) ;

	if (query_ok)
		display_result (slave, discid);

	return 0;
}

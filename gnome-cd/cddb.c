/*
 * cddb.c
 *
 * Copyright (C) 2001 Iain Holmes
 * Authors: Iain Holmes <iain@ximian.com>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <bonobo/bonobo-listener.h>

#include <cddb-slave-client.h>
#include <GNOME_Media_CDDBSlave2.h>

#include <libgnome/gnome-util.h>

#include "gnome-cd.h"
#include "cdrom.h"
#include "cddb.h"

static void
load_cddb_data (GnomeCD *gcd,
		const char *discid)
{
	char *filename, *pathname;

	pathname = gnome_util_prepend_user_home (".cddbslave");
	filename = g_concat_dir_and_file (pathname, discid);
	g_free (pathname);

	g_print ("Loading %s\n", filename);

	g_free (filename);
}

static void
cddb_listener_event_cb (BonoboListener *listener,
			const char *name,
			const BonoboArg *arg,
			CORBA_Environment *ev,
			GnomeCD *gcd)
{
	GNOME_Media_CDDBSlave2_QueryResult *qr;

	g_print ("Got CDDB data\n");
	qr = arg->_value;

	g_print ("Got results for %s, %d\n", qr->discid, qr->result);
	switch (qr->result) {
	case GNOME_Media_CDDBSlave2_OK:
		load_cddb_data (gcd, qr->discid);
		break;

	case GNOME_Media_CDDBSlave2_REQUEST_PENDING:
		/* Do nothing really */
		break;

	case GNOME_Media_CDDBSlave2_ERROR_CONTACTING_SERVER:
		g_warning ("Could not contact CDDB server");
		break;

	case GNOME_Media_CDDBSlave2_ERROR_RETRIEVING_DATA:
		g_warning ("Error downloading data");
		break;

	case GNOME_Media_CDDBSlave2_MALFORMED_DATA:
		g_warning ("Malformed data");
		break;

	case GNOME_Media_CDDBSlave2_IO_ERROR:
		g_warning ("Generic IO error");
		break;

	default:
		break;
	}
}

static GnomeCDDiscInfo *
cddb_make_disc_info (GnomeCDRomCDDBData *data)
{
	GnomeCDDiscInfo *discinfo;

	discinfo = g_new (GnomeCDDiscInfo, 1);
	discinfo->discid = data->discid;
	discinfo->name = NULL;
	discinfo->artist = NULL;
	discinfo->track_names = g_new0 (char *, data->ntrks);

	return discinfo;
}

void
cddb_get_query (GnomeCD *gcd)
{
	GnomeCDRomCDDBData *data;
	CDDBSlaveClient *slave;
	BonoboListener *listener;
	char *discid;
	char *offsets = NULL;
	int i;

	if (gnome_cdrom_get_cddb_data (gcd->cdrom, &data, NULL) == FALSE) {
		g_print ("Eeeeek");
		return;
	}

	gcd->disc_info = cddb_make_disc_info (data);
	discid = g_strdup_printf ("%08lx", data->discid);
	for (i = 0; i < data->ntrks; i++) {
		char *tmp;

		tmp = g_strdup_printf ("%u ", data->offsets[i]);
		if (offsets == NULL) {
			offsets = tmp;
		} else {
			offsets = g_strconcat (offsets, tmp, NULL);
			g_free (tmp);
		}
	}

	/* Remove the last space */
	offsets[strlen (offsets) - 1] = 0;

	slave = cddb_slave_client_new ();
	listener = bonobo_listener_new (NULL, NULL);
	g_signal_connect (G_OBJECT (listener), "event-notify",
			  G_CALLBACK (cddb_listener_event_cb), gcd);

	cddb_slave_client_add_listener (slave, listener);

	g_print ("Sending Query:-------\n");
	g_print ("Disc ID: %s\nNTrks: %d\n", discid, data->ntrks);
	g_print ("Offsets: %s\nNSecs: %d\n", offsets, data->nsecs);

	cddb_slave_client_query (slave, discid, data->ntrks, offsets, 
				 data->nsecs, "GnomeCD", VERSION);
	
	gnome_cdrom_free_cddb_data (data);
}

int
cddb_sum (int n)
{
	char buf[12], *p;
	int ret = 0;
	
	/* This is what I get for copying TCD code */
	sprintf (buf, "%u", n);
	for (p = buf; *p != '\0'; p++) {
		ret += (*p - '0');
	}

	return ret;
}

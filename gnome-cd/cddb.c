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

#include "gnome-cd.h"
#include "cdrom.h"
#include "cddb.h"

static void
cddb_listener_event_cb (BonoboListener *listener,
			const char *name,
			const BonoboArg *arg,
			CORBA_Environment *ev,
			GnomeCD *gcd)
{
	g_print ("Got CDDB data\n");
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

	discid = g_strdup_printf ("%08lx", data->discid);
	for (i = 0; i <= data->ntrks; i++) {
		char *tmp;

		tmp = g_strdup_printf ("%u ", data->offsets[i]);
		if (offsets == NULL) {
			offsets = tmp;
		} else {
			offsets = g_strconcat (offsets, tmp, NULL);
			g_free (tmp);
		}
	}

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

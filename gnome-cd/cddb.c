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

static GHashTable *cddb_cache;

static void
load_cddb_data (GnomeCD *gcd,
		const char *discid)
{
	GnomeCDDiscInfo *info;
	char *filename, *pathname;
	char line[4096];
	FILE *handle;

	pathname = gnome_util_prepend_user_home (".cddbslave");
	filename = g_concat_dir_and_file (pathname, discid);
	g_free (pathname);

	info = g_hash_table_lookup (cddb_cache, discid);
	if (info == NULL) {
		g_warning ("No cache for %s\n", discid);
		return;
	} else {
		gcd->disc_info = info;
	}

	g_print ("Loading %s\n", filename);
	handle = fopen (filename, "r");
	if (handle == NULL) {
		g_warning ("No such file %s", filename);
		g_free (filename);
		return;
	}

	while (fgets (line, 4096, handle)) {
		char *end;

		if (*line == 0 || *line == '#') {
			continue;
		}

		if (*line == '.') {
			break;
		}

		if (strncmp (line, "DISCID", 6) == 0) {
			g_print ("Found discid: %s\n", discid);
		} else if (strncmp (line, "DTITLE", 6) == 0) {
			char *title = line + 7;
			char *album, *artist, *div;

			g_print ("Found title %s\n", title);
			
			div = strstr (title, " / ");
			if (div == NULL) {
				g_print ("Duff line? %s\n", line);
				continue;
			}

			*div = 0;
			info->artist = g_strdup (title);
			info->title = g_strdup (div + 3);
		} else if (strncmp (line, "TTITLE", 6) == 0) {
			char *name;
			int number;

			name = strchr (line+6, '=');
			if (name == NULL) {
				g_print ("Duff line...? %s\n", line);
				continue;
			}

			*name = 0;
			name = name + 1;
			number = atoi (line + 6);

			if (number > info->ntracks) {
				continue;
			}

			g_print ("Found %d: %s\n", number, name);
			info->tracknames[number] = g_strdup (name);
		} else if (strncmp (line, "EXTD", 4) == 0) {
		} else if (strncmp (line, "EXTT", 4) == 0) {
		} else if (strncmp (line, "PLAYORDER", 9) == 0) {
		} else if (isdigit (*line)) {
			char *space;

			space = strchr (line, ' ');
			if (space != NULL) {
				*space = 0;
			}

			if (atoi (line) == 210) {
				g_print ("All ok\n");
			} else {
				g_print ("Error %d\n", atoi (line));
				gcd->disc_info = NULL;
			}
		}
	}

	g_print ("Loaded\n");
	fclose (handle);
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
		gcd->disc_info = NULL;
		break;

	case GNOME_Media_CDDBSlave2_ERROR_CONTACTING_SERVER:
		g_warning ("Could not contact CDDB server");
		gcd->disc_info = NULL;
		break;

	case GNOME_Media_CDDBSlave2_ERROR_RETRIEVING_DATA:
		g_warning ("Error downloading data");
		gcd->disc_info = NULL;
		break;

	case GNOME_Media_CDDBSlave2_MALFORMED_DATA:
		g_warning ("Malformed data");
		gcd->disc_info = NULL;
		break;

	case GNOME_Media_CDDBSlave2_IO_ERROR:
		g_warning ("Generic IO error");
		gcd->disc_info = NULL;
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
	discinfo->discid = g_strdup_printf ("%08lx", data->discid);
	discinfo->title = NULL;
	discinfo->artist = NULL;
	discinfo->ntracks = data->ntrks;
	discinfo->tracknames = g_new0 (char *, data->ntrks);

	return discinfo;
}

void
cddb_get_query (GnomeCD *gcd)
{
	GnomeCDRomCDDBData *data;
	GnomeCDDiscInfo *info;
	CDDBSlaveClient *slave;
	BonoboListener *listener;
	char *discid;
	char *offsets = NULL;
	int i;

	if (cddb_cache == NULL) {
		cddb_cache = g_hash_table_new (g_str_hash, g_str_equal);
		/* cddb_preload_cache (); */
	}

	if (gnome_cdrom_get_cddb_data (gcd->cdrom, &data, NULL) == FALSE) {
		g_print ("Eeeeek");
		return;
	}

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

	info = g_hash_table_lookup (cddb_cache, discid);
	if (info != NULL) {
		gcd->disc_info = info;
		return;
	} else {
		info = cddb_make_disc_info (data);
		g_hash_table_insert (cddb_cache, info->discid, info);
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

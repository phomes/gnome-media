/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 *  Authors: Iain Holmes <iain@ximian.com>
 *
 *  Copyright 2002 , Iain Holmes.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include "cddb-slave.h"

gboolean
cddb_entry_parse_file (CDDBEntry *entry,
		       const char *filename)
{
	FILE *handle;
	char line[4096];
	gboolean need_topline = TRUE;
	
	handle = fopen (filename, "r");
	if (handle == NULL) {
		return FALSE;
	}

	while (fgets (line, 4096, handle)) {
		char *end;
		char **vector;
		GString *string;

		if (isdigit (*line) && need_topline == TRUE) {
			/* Save topline so we can regenerate things later.
			   and so we know the discid. */
			entry->topline = g_strsplit (line, " ", 4);
			need_topline = FALSE;
			continue;
		}

		if (*line == '#') {
			/* Save comments to rebuild the file later. */
			entry->comments = g_list_append (entry->comments,
							 g_strdup (line));
			continue;
		}
		
		if (*line == 0 || isdigit (*line)) {
			continue;
		}

		if (*line == '.') {
			break;
		}

		/* Strip newlines */
		line[strlen (line) - 1] = 0;
		/* Check for \r */
		end = strchr (line, '\r');
		if (end != NULL) {
			*end = 0;
		}

		vector = g_strsplit (line, "=", 2);
		if (vector == NULL) {
			continue;
		}
		
		/* See if we have this ID */
		string = g_hash_table_lookup (entry->fields, vector[0]);
		if (string == NULL) {
			string = g_string_new (vector[1]);
			g_hash_table_insert (entry->fields, g_strdup (vector[0]), string);
		} else {
			g_string_append (string, vector[1]);
		}

		g_strfreev (vector);
	}

	return TRUE;
}

#define XMCD_INDICATOR "xmcd"
#define XMCD_INDICATOR_LEN 4

#define TRACK_OFFSET_INDICATOR "Track frame offsets:"
#define TRACK_OFFSET_INDICATOR_LEN 20

#define DISC_LENGTH_INDICATOR "Disc length:"
#define DISC_LENGTH_INDICATOR_LEN 12

#define REVISION_INDICATOR "Revision:"
#define REVISION_INDICATOR_LEN 9

static void
parse_comments (CDDBEntry *entry)
{
	GList *comments;
	gboolean parsing_offsets = FALSE;
	int trackno = 0;
	
	for (comments = entry->comments; comments; comments = comments->next) {
		char *line = comments->data;
		
		line++; /* Move past the # */
		while (g_ascii_isspace (*line)) {
			line++; /* Move past the white space */
		}

		if (parsing_offsets == TRUE) {
			g_print ("Found (%d) %s\n", trackno, line);
			entry->offsets[trackno] = atoi (line);
			trackno++;

			if (trackno >= entry->ntrks) {
				parsing_offsets = FALSE;
			}
		}
			
		if (strncmp (XMCD_INDICATOR, line, XMCD_INDICATOR_LEN) == 0) {
			g_print ("Is xmcd file\n");
			parsing_offsets = FALSE;
			continue;
		}
		
		if (strncmp (TRACK_OFFSET_INDICATOR, line, TRACK_OFFSET_INDICATOR_LEN) == 0) {
			g_print ("Found track offsets\n");
			parsing_offsets = TRUE;
			trackno = 0;
			continue;
		}

		if (strncmp (DISC_LENGTH_INDICATOR, line, DISC_LENGTH_INDICATOR_LEN) == 0) {
			parsing_offsets = FALSE;
			line += (DISC_LENGTH_INDICATOR_LEN + 1); /* Past white space */
			entry->disc_length = atoi (line);
			g_print ("Found disc length: %d\n", entry->disc_length);
			continue;
		}

		if (strncmp (REVISION_INDICATOR, line, REVISION_INDICATOR_LEN) == 0) {
			parsing_offsets = FALSE;
			line += (REVISION_INDICATOR_LEN + 1); /* Past white space */
			entry->revision = atoi (line);
			g_print ("Found revision: %d\n", entry->revision);
			continue;
		}
	}
}

#define CD_FRAMES 75

static void
calculate_lengths (CDDBEntry *entry)
{
	int i;
	
	for (i = 0; i < entry->ntrks; i++) {
		int start, finish;
		int length;

		start = entry->offsets[i];
		if (i == entry->ntrks - 1) {
			finish = entry->disc_length * CD_FRAMES;
		} else {
			finish = entry->offsets[i + 1];
		}

		length = finish - start;

		entry->lengths[i] = (length / CD_FRAMES) - 1;
	}
}

CDDBEntry *
cddb_entry_new (const char *discid,
		int ntrks,
		const char *offsets,
		int nsecs)
{
	CDDBEntry *entry;
	char **offset_vector;
	int i;

	entry = g_new0 (CDDBEntry, 1);

	entry->discid = g_strdup (discid);
	entry->ntrks = ntrks;
  	entry->offsets = g_new (int, entry->ntrks);
	entry->lengths = g_new (int, entry->ntrks);
	offset_vector = g_strsplit (offsets, " ", entry->ntrks);
	for (i = 0; i < entry->ntrks; i++) {
		entry->offsets[i] = atoi (offset_vector[i]);
	}
	g_strfreev (offset_vector);
	
	entry->disc_length = nsecs;

	calculate_lengths (entry);
	entry->fields = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
	return entry;
}

static void
is_track (gpointer key,
	  gpointer value,
	  gpointer data)
{
	int *t = (int *) data;

	if (strncasecmp (key, "TTITLE", 6) == 0) {
		(*t)++;
	}
}

static int
count_tracks (CDDBEntry *entry)
{
	int ntrks = 0;

	g_hash_table_foreach (entry->fields, is_track, &ntrks);
	return ntrks;
}

CDDBEntry *
cddb_entry_new_from_file (const char *filename)
{
	CDDBEntry *entry;

	entry = g_new0 (CDDBEntry, 1);

	entry->fields = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
	if (cddb_entry_parse_file (entry, filename) == FALSE) {
		g_hash_table_destroy (entry->fields);
		g_strfreev (entry->topline);
		g_free (entry);
		return FALSE;
	}

	entry->discid = g_strdup (entry->topline[2]);
	entry->ntrks = count_tracks (entry);
	entry->offsets = g_new (int, entry->ntrks);
	entry->lengths = g_new (int, entry->ntrks);
	
	parse_comments (entry);

	calculate_lengths (entry);
	
	return entry;
}

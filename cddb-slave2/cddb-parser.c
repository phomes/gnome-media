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
	
	handle = fopen (filename, "r");
	if (handle == NULL) {
		return FALSE;
	}

	while (fgets (line, 4096, handle)) {
		char *end;
		char *id, *data;
		GString *string;
		
		if (*line == 0 || *line == '#' || isdigit (*line)) {
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

		end = strchr (line, '=');
		if (end == NULL) {
			continue;
		}
		id = g_strndup (line, end - line);
		/* See if we have this ID */
		string = g_hash_table_lookup (entry->fields, id);
		if (string == NULL) {
			string = g_string_new (end + 1);
			g_hash_table_insert (entry->fields, id, string);

			/* Don't want to free id here, as it's the key */
		} else {
			g_string_append (string, end + 1);
			/* Free id here as we don't need it */
			g_free (id);
		}
	}

	return TRUE;
}

CDDBEntry *
cddb_entry_new (const char *discid,
		int ntrks,
		const char *offsets,
		int nsecs)
{
	CDDBEntry *entry;

	entry = g_new0 (CDDBEntry, 1);

	entry->discid = g_strdup (discid);
	entry->ntrks = ntrks;
	entry->offsets = g_strdup (offsets);
	entry->nsecs = nsecs;
	
	entry->fields = g_hash_table_new (g_str_hash, g_str_equal);
	return entry;
}

/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 *  Authors: Iain Holmes <iain@prettypeople.org>
 *
 * Copyright 2002, 2003, 2004, 2005 Iain Holmes
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 * 4th Februrary 2005: Christian Schaller: changed license to LGPL with 
 * permission of Iain Holmes, Ronald Bultje, Leontine Binchy (SUN), Johan Dahlin *  and Joe Marcus Clarke
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gconf/gconf-client.h>
#include <gnome.h>

#include <gst/gst.h>

#include "gsr-window.h"
#include "egg-recent-model.h"

extern void gnome_media_profiles_init (GConfClient *conf);
 
static GList *windows = NULL;

GConfClient *gconf_client = NULL;

EggRecentModel *recent_model = NULL;

static gboolean
delete_event_cb (GSRWindow *window,
		 gpointer data)
{
	if (! gsr_window_is_saved (window)) {
		close_confirmation_dialog (window);
		return TRUE;
	}

	return FALSE;
}

void
window_destroyed (GtkWidget *window,
		  gpointer data)
{
	windows = g_list_remove (windows, window);

	if (windows == NULL) {
		gtk_main_quit ();
	}
}

void
gsr_quit (void) 
{
	GList *p;

	for (p = windows; p;) {
		GSRWindow *window = p->data;

		/* p is set here instead of in the for statement,
		   because by the time we get back to the loop,
		   p will be invalid */
		p = p->next;

		if (! gsr_window_is_saved (window))
			close_confirmation_dialog (window);
		else
			gsr_window_close (window);
	}
}

#define RECENT_HISTORY_LEN 5

static void
init_recent (void)
{
	/* global recent model */
	recent_model = egg_recent_model_new (EGG_RECENT_MODEL_SORT_MRU);

	egg_recent_model_set_filter_groups (recent_model, 
					    "gnome-sound-recorder",
					    NULL);

	egg_recent_model_set_filter_uri_schemes (recent_model, "file", NULL);
	egg_recent_model_set_limit (recent_model, RECENT_HISTORY_LEN);
}

void
gsr_add_recent (gchar *filename)
{
	char *uri;
	EggRecentItem *item;

	g_return_if_fail (filename != NULL);

	uri = g_filename_to_uri (filename, NULL, NULL);

	if (uri) {
		item = egg_recent_item_new_from_uri (uri);
		g_return_if_fail (item != NULL);

		egg_recent_item_add_group (item, "gnome-sound-recorder");
		egg_recent_model_add_full (recent_model, item);

		egg_recent_item_unref (item);
	}

	g_free (uri);
}

gint gsr_sample_count = 1;

GtkWidget *
gsr_open_window (const char *filename)
{
	GtkWidget *window;
	char *utf8_name;
	char *name;

	if (filename == NULL) {
		/* Translator comment: untitled here implies that
		 * there is no active sound sample. Any newly
		 * recorded samples will be saved to disk with this
		 * name as default value. */
		if (gsr_sample_count == 1) {
			utf8_name = g_strdup (_("Untitled"));
		} else {
			utf8_name = g_strdup_printf (_("Untitled-%d"), gsr_sample_count);
		}
		name = g_filename_from_utf8 (utf8_name, -1, NULL, NULL, NULL);
		g_free (utf8_name);
		++gsr_sample_count;
	} else {
		name = g_strdup (filename);
	}

	window = GTK_WIDGET (gsr_window_new (name));
	g_free (name);

	g_signal_connect (G_OBJECT (window), "delete-event",
			  G_CALLBACK (delete_event_cb), NULL);

	g_signal_connect (G_OBJECT (window), "destroy",
			  G_CALLBACK (window_destroyed), NULL);

	windows = g_list_prepend (windows, window);
	gtk_widget_show (window);

	return window;
}

int
main (int argc,
      char **argv)
{
	gchar **filenames = NULL;
	/* this is necessary because someone apparently forgot to add a
	 * convenient way to get the remaining arguments to the GnomeProgram
	 * API when adding the GOption stuff to it ... */
	const GOptionEntry entries[] = {
		{ G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &filenames,
		"Special option that collects any remaining arguments for us" },
		{ NULL, }
	};

	GOptionContext *ctx;
	GnomeProgram *program;

	g_thread_init (NULL);

	/* Init gettext */
	bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	ctx = g_option_context_new ("gnome-sound-recorder");
	g_option_context_add_group (ctx, gst_init_get_option_group ());
	g_option_context_add_main_entries (ctx, entries, GETTEXT_PACKAGE);
	program = gnome_program_init ("gnome-sound-recorder", VERSION,
	                              LIBGNOMEUI_MODULE, argc, argv,
	                              GNOME_PARAM_GOPTION_CONTEXT, ctx,
	                              GNOME_PARAM_HUMAN_READABLE_NAME,
	                              "GNOME Sound Recorder",
	                              GNOME_PARAM_APP_DATADIR, DATADIR,
	                              NULL);

	init_recent ();

	gtk_window_set_default_icon_name ("gnome-sound-recorder");

	/* use it like a singleton */
	gconf_client = gconf_client_get_default ();

	/* init gnome-media-profiles */
	gnome_media_profiles_init (gconf_client);

	if (filenames != NULL && filenames[0] != NULL) {
		guint i, num;

		num = g_strv_length (filenames);
		for (i = 0; i < num; ++i) {
			gsr_open_window (filenames[i]);
		}
	} else {
			gsr_open_window (NULL);
	}

	if (filenames) {
		g_strfreev (filenames);
	}

	gtk_main ();

	return 0;
}

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
#include "gst/manager.h"

extern void gnome_media_profiles_init (GConfClient *conf);
 
static GList *windows = NULL;

GConfClient *gconf_client = NULL;

static void
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
		gsr_window_close (window);
	}
}

gint sample_count = 1;

GtkWidget *
gsr_open_window (const char *filename)
{
	GtkWidget *window;
	char *name;

	if (filename == NULL) {
		/* Translator comment: untitled here implies that
		 * there is no active sound sample. Any newly
		 * recorded samples will be saved to disk with this
		 * name as default value. */
		if (sample_count == 1) {
			name = g_strdup (_("Untitled"));
		} else {
			name = g_strdup_printf (_("Untitled-%d"),
						sample_count);
		}
		sample_count++;
	} else {
		name = g_strdup (filename);
	}

	window = GTK_WIDGET (gsr_window_new (name));
	g_free (name);
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
	GnomeProgram *program;
	poptContext pctx;
	GValue value = {0, };
	char **args = NULL;

	static struct poptOption gsr_options[] = {
		{ NULL, '\0', POPT_ARG_INCLUDE_TABLE, NULL, 0, "GStreamer", NULL },
		{ NULL, 'p', POPT_ARG_NONE, NULL, 1, N_("Dummy option"), NULL },
		POPT_TABLEEND
	};

	/* Init gettext */
	bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	/* init gstreamer */
	gsr_options[0].arg = (void *) gst_init_get_popt_table ();

	/* Init GNOME */
	program = gnome_program_init ("gnome-sound-recorder", VERSION,
				      LIBGNOMEUI_MODULE,
				      argc, argv,
				      GNOME_PARAM_POPT_TABLE, gsr_options,
				      GNOME_PARAM_HUMAN_READABLE_NAME,
				      "GNOME Sound Recorder",
				      GNOME_PARAM_APP_DATADIR, DATADIR,
				      NULL);

	if (!gst_scheduler_factory_get_default_name ()) {
		GtkWidget *dialog;

		dialog = gtk_message_dialog_new (NULL,
						 0,
						 GTK_MESSAGE_ERROR,
						 GTK_BUTTONS_CLOSE,
						 _("Registry is not present or it is corrupted, please update it by running gst-register"));

		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);

		exit (1);
	}
	gst_rec_elements_init ();

	/* use it like a singleton */
	gconf_client = gconf_client_get_default ();

	gtk_window_set_default_icon_name ("gnome-grecord");

        /* init gnome-media-profiles */
        gnome_media_profiles_init (gconf_client);

	/* Get the args */
	g_value_init (&value, G_TYPE_POINTER);
	g_object_get_property (G_OBJECT (program),
			       GNOME_PARAM_POPT_CONTEXT, &value);
	pctx = g_value_get_pointer (&value);
	g_value_unset (&value);

	args = (char **) poptGetArgs (pctx);
	if (args == NULL) {
		gsr_open_window (NULL);
	} else {
		int i;

		for (i = 0; args[i]; i++) {
			gsr_open_window (args[i]);
		}
	}

	poptFreeContext (pctx);

	gtk_main ();

	return 0;
}


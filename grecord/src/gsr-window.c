/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 *  Authors: Iain Holmes <iain@prettypeople.org>
 *           Johan Dahlin <johan@gnome.org>
 *
 *  Copyright 2002 Iain Holmes
 *
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
 * permission of Iain Holmes, Ronald Bultje, Leontine Binchy (SUN), Johan Dalhin * and Joe Marcus Clarke
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <libgnome/gnome-help.h>
#include <libgnomevfs/gnome-vfs.h>
#include <gconf/gconf-client.h>
#include <gst/gst.h>
#include <gst/gconf/gconf.h>

#include <profiles/gnome-media-profiles.h>

#include "egg-recent-view.h"
#include "egg-recent-view-gtk.h"
#include "egg-recent-model.h"
#include "egg-recent-util.h"

#include "gsr-window.h"

extern GtkWidget * gsr_open_window (const char *filename);
extern void gsr_quit (void);

extern GConfClient *gconf_client;

extern EggRecentModel *recent_model;
extern void gsr_add_recent (gchar *filename);

#define GCONF_DIR           "/apps/gnome-sound-recorder/"
#define KEY_OPEN_DIR        GCONF_DIR "system-state/open-file-directory"
#define KEY_SAVE_DIR        GCONF_DIR "system-state/save-file-directory"
#define KEY_LAST_PROFILE_ID GCONF_DIR "last-profile-id"
#define EBUSY_TRY_AGAIN     3000    /* Empirical data */

typedef struct _GSRWindowPipeline {
	GstElement *pipeline;

	GstElement *src, *sink;
	guint32 state_change_id;
} GSRWindowPipeline;

enum {
	PROP_0,
	PROP_LOCATION
};

static GtkWindowClass *parent_class = NULL;

#define GSR_WINDOW_GET_PRIVATE(object)(G_TYPE_INSTANCE_GET_PRIVATE ((object), GSR_TYPE_WINDOW, GSRWindowPrivate))

struct _GSRWindowPrivate {
	GtkWidget *main_vbox;
	GtkWidget *scale;
	GtkWidget *profile;
	GtkWidget *rate, *time_sec, *format, *channels;
	GtkWidget *name_label;
	GtkWidget *length_label;
	gulong seek_id;

	GtkUIManager *ui_manager;
	GtkActionGroup *action_group;
	EggRecentViewGtk *recent_view;

	/* statusbar */
	GtkWidget *statusbar;
	guint status_message_cid;
	guint tip_message_cid;

	/* Pipelines */
	GSRWindowPipeline *play, *record;
	char *filename, *record_filename;
        char *extension;
	char *working_file; /* Working file: Operations only occur on the
			       working file. The result of that operation then
			       becomes the new working file. */
	int record_fd;

	/* File info */
	int len_secs; /* In seconds */
	int get_length_attempts;

	int n_channels, bitrate, samplerate;
	gboolean has_file;
	gboolean saved;
	gboolean dirty;
	gboolean seek_in_progress;

	guint32 tick_id; /* tick_callback timeout ID */
	guint32 record_id; /* record idle callback timeout ID */
	guint32 gstenc_id; /* encode idle callback iterator ID */
};

static GSRWindowPipeline * make_record_pipeline    (GSRWindow         *window);
static GSRWindowPipeline * make_play_pipeline      (GSRWindow         *window);

static void
show_error_dialog (GtkWindow *window, gchar *error)
{
	GtkWidget *dialog;

	dialog = gtk_message_dialog_new (window,
					 GTK_DIALOG_DESTROY_WITH_PARENT,
					 GTK_MESSAGE_ERROR,
					 GTK_BUTTONS_CLOSE,
					 error);
	
	gtk_dialog_run (GTK_DIALOG (dialog));
	gtk_widget_destroy (dialog);
}

static void
shutdown_pipeline (GSRWindowPipeline *pipe)
{
	if (pipe->state_change_id > 0) {
		g_signal_handler_disconnect (G_OBJECT (pipe->pipeline),
					     pipe->state_change_id);
	}
	gst_element_set_state (pipe->pipeline, GST_STATE_NULL);
	gst_object_unref (GST_OBJECT (pipe->pipeline));	
}

static char *
seconds_to_string (guint seconds)
{
	int hour, min, sec;
	
	min = (seconds / 60);
	hour = min / 60;
	min -= (hour * 60);
	sec = seconds - ((hour * 3600) + (min * 60));

	if (hour > 0) {
		return g_strdup_printf ("%d:%02d:%02d", hour, min, sec);
	} else {
		return g_strdup_printf ("%d:%02d", min, sec);
	}
}	

static char *
seconds_to_full_string (guint seconds)
{
	int hour, min, sec;

	min = (seconds / 60);
	hour = (min / 60);
	min -= (hour * 60);
	sec = seconds - ((hour * 3600) + (min * 60));

	if (hour > 0) {
		if (min > 0) {
			if (sec > 0) {
				return g_strdup_printf ("%d %s %d %s %d %s",
							hour, hour > 1 ? _("hours") : _("hour"),
							min, min > 1 ? _("minutes") : _("minute"),
							sec, sec > 1 ? _("seconds") : _("second"));
			} else {
				return g_strdup_printf ("%d %s %d %s",
							hour, hour > 1 ? _("hours") : _("hour"),
							min, min > 1 ? _("minutes") : _("minute"));
			}
		} else {
			if (sec > 0) {
				return g_strdup_printf ("%d %s %d %s",
							hour, hour > 1 ? _("hours") : _("hour"),
							sec, sec > 1 ? _("seconds") : _("second"));
			} else {
				return g_strdup_printf ("%d %s",
							hour, hour > 1 ? _("hours") : _("hour"));
			}
		}
	} else {
		if (min > 0) {
			if (sec > 0) {
				return g_strdup_printf ("%d %s %02d %s",
							min, min > 1 ? _("minutes") : _("minute"),
							sec, sec > 1 ? _("seconds") : _("second"));
			} else {
				return g_strdup_printf ("%d %s",
							min, min > 1 ? _("minutes") : _("minute"));
			}
		} else {
			if (sec == 0) {
				return g_strdup_printf ("%d %s",
							sec, _("seconds"));
			} else {
				return g_strdup_printf ("%d %s",
							sec, sec > 1 ? _("seconds") : _("second"));
			}
		}
	}

	return NULL;
}

set_action_sensitive (GSRWindow  *window,
		      const char *name,
		      gboolean    sensitive)
{
	GtkAction *action = gtk_action_group_get_action (window->priv->action_group,
							 name);
	gtk_action_set_sensitive (action, sensitive);
}

static void
file_new_cb (GtkAction *action,
	     GSRWindow *window)
{
	gsr_open_window (NULL);
}

static void
file_chooser_open_response_cb (GtkDialog *file_chooser,
			       int response_id,
			       GSRWindow *window)
{
	char *name;
	char *dirname;

	if (response_id != GTK_RESPONSE_OK)
		goto out;
	
	name = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (file_chooser));
	if (name == NULL)
		goto out;
		
	dirname = g_path_get_dirname (name);
	gconf_client_set_string (gconf_client, KEY_OPEN_DIR, dirname, NULL);
	g_free (dirname);

	if (window->priv->has_file == TRUE) {
		/* Just open a new window with the file */
		gsr_open_window (name);
	} else {
		/* Set the file in this window */
		g_object_set (G_OBJECT (window), "location", name, NULL);
	}
		
	g_free (name);
	
 out:
	gtk_widget_destroy (GTK_WIDGET (file_chooser));
}

static void
file_open_cb (GtkAction *action,
	      GSRWindow *window)
{
	GtkWidget *file_chooser;
	char *directory;

	file_chooser = gtk_file_chooser_dialog_new (_("Open a File"),
						    GTK_WINDOW (window),
						    GTK_FILE_CHOOSER_ACTION_OPEN,
						    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						    GTK_STOCK_OPEN, GTK_RESPONSE_OK,
						    NULL);

	directory = gconf_client_get_string (gconf_client, KEY_OPEN_DIR, NULL);
	if (directory != NULL && *directory != 0) {
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (file_chooser), directory);
	}
	g_free (directory);

	g_signal_connect (G_OBJECT (file_chooser), "response",
			  G_CALLBACK (file_chooser_open_response_cb), window);
	gtk_widget_show (file_chooser);
}

static void
file_open_recent_cb (EggRecentViewGtk *view,
		     EggRecentItem *item,
		     GSRWindow *window)
{
	gchar *uri;
	gchar *filename;

	uri = egg_recent_item_get_uri (item);
	g_return_if_fail (uri != NULL);

	if (!g_str_has_prefix (uri, "file://"))
		return;

	filename = g_filename_from_uri (uri, NULL, NULL);
	if (filename == NULL)
		goto out;

	if (!g_file_test (filename, G_FILE_TEST_EXISTS)) {
		gchar *filename_utf8;
		GtkWidget *dlg;

		filename_utf8 = g_filename_to_utf8 (filename, -1, NULL, NULL, NULL);
		dlg = gtk_message_dialog_new (GTK_WINDOW (window),
					      GTK_DIALOG_MODAL,
					      GTK_MESSAGE_ERROR,
					      GTK_BUTTONS_OK,
					      _("Unable to load file:\n%s"), filename_utf8);

		gtk_widget_show (dlg);
		gtk_dialog_run (GTK_DIALOG (dlg));
		gtk_widget_destroy (dlg);

		egg_recent_model_delete (recent_model, uri);

		g_free (filename_utf8);
		goto out;
  	}

	if (window->priv->has_file == TRUE) {
		/* Just open a new window with the file */
		gsr_open_window (filename);
	} else {
		/* Set the file in this window */
		g_object_set (G_OBJECT (window), "location", filename, NULL);
	}

 out:
	g_free (filename);
	g_free (uri);
}

struct _eos_data {
	GSRWindow *window;
	char *location;
	GstElement *pipeline;
};

static gboolean
eos_done (struct _eos_data *ed)
{
	GSRWindow *window = ed->window;

 	gst_element_set_state (ed->pipeline, GST_STATE_NULL);
	if (window->priv->gstenc_id > 0) {
	    g_source_remove (window->priv->gstenc_id);
	    window->priv->gstenc_id = 0;
	}
 
	g_object_set (G_OBJECT (window),
		      "location", ed->location,
		      NULL);


	gdk_window_set_cursor (window->priv->main_vbox->window, NULL);

	set_action_sensitive (window, "Stop", FALSE);
	set_action_sensitive (window, "Play", TRUE);
	set_action_sensitive (window, "Record", TRUE);
	set_action_sensitive (window, "FileSave", FALSE);
	set_action_sensitive (window, "FileSaveAs", TRUE);
	gtk_widget_set_sensitive (window->priv->scale, TRUE);

	gtk_statusbar_pop (GTK_STATUSBAR (window->priv->statusbar),
			   window->priv->status_message_cid);
	gtk_statusbar_push (GTK_STATUSBAR (window->priv->statusbar),
			    window->priv->status_message_cid,
			    _("Ready"));

	g_free (ed);

	return FALSE;
}

static GstElement *
gst_element_get_child (GstElement* element, const gchar *name)
{
	const GList *kids = GST_BIN (element)->children;
	GstElement *sink;

	while (kids && !(strcmp (gst_object_get_name (GST_OBJECT (kids->data)), name)))
		kids = kids->next;
	g_assert (kids);
	sink = kids->data;

	return sink;
}

#if 0
static gboolean
cb_iterate (GstBin  *bin,
	    gpointer data)
{
	src = gst_element_get_child (bin, "sink");
	sink = gst_element_get_child (bin, "sink");
	
	if (src && sink) {
		gint64 pos, tot, enc;
		GstFormat fmt = GST_FORMAT_BYTES;

		gst_element_query (src, GST_QUERY_POSITION, &fmt, &pos);
		gst_element_query (src, GST_QUERY_TOTAL, &fmt, &tot);
		gst_element_query (sink, GST_QUERY_POSITION, &fmt, &enc);

		g_print ("Iterate: %lld/%lld -> %lld\n", pos, tot, enc);
	} else
		g_print ("Iterate ?\n");

	/* we don't do anything here */
	return FALSE;
}
#endif

static gboolean
is_set_ebusy_timeout (GSRWindow *window)
{
	if (g_object_get_data (G_OBJECT (window), "ebusy_timeout"))
		return TRUE;

	return FALSE;
}

static void
set_ebusy_timeout (GSRWindow *window, gpointer data)
{
	g_object_set_data (G_OBJECT (window), "ebusy_timeout", data);
}

static gboolean
handle_ebusy_error (GSRWindow *window)
{
	GSRWindowPrivate *priv = window->priv;

	/* FIXME: which pipeline to reset state on? */
	if (priv->play) {
		gst_element_set_state (priv->play->pipeline, GST_STATE_NULL);
		gst_element_set_state (priv->play->pipeline, GST_STATE_PLAYING);
	} else if (priv->record) {
		gst_element_set_state (priv->record->pipeline, GST_STATE_NULL);
		gst_element_set_state (priv->record->pipeline, GST_STATE_PLAYING);
	} else {
		g_warning ("Don't know which pipeline to reset");
	}

	/* Try only once */
	return FALSE;
}

static void
pipeline_error_cb (GstElement *parent,
		   GstElement *cause,
		   GError     *error,
		   gchar      *debug,
		   gpointer    data)
{
	GSRWindow *window = data;
	struct _eos_data *ed;
	GstElement *sink;

	g_return_if_fail (GTK_IS_WINDOW (window));
	
	if (error->code == GST_RESOURCE_ERROR_BUSY) {
		if (! is_set_ebusy_timeout (window)) {
			g_timeout_add (EBUSY_TRY_AGAIN, (GSourceFunc) handle_ebusy_error, window);
			set_ebusy_timeout (window, GUINT_TO_POINTER (TRUE));

			set_action_sensitive (window, "FileSave", FALSE);
			set_action_sensitive (window, "FileSaveAs", FALSE);
			set_action_sensitive (window, "Play", FALSE);
			set_action_sensitive (window, "Record", FALSE);
			return;
		}
	}

	set_ebusy_timeout (window, NULL);

	show_error_dialog (GTK_WINDOW (window), error->message);

	ed = g_new (struct _eos_data, 1);
	ed->window = window;
	ed->pipeline = parent;

	sink = gst_element_get_child (parent, "sink");
	g_object_get (G_OBJECT (sink),
		      "location", &ed->location,
		      NULL);

	/*g_idle_add ((GSourceFunc) eos_done, ed);*/
	eos_done (ed);
}

static GtkWidget *
gsr_dialog_add_button (GtkDialog *dialog,
		       const gchar *text,
		       const gchar *stock_id,
		       gint response_id)
{
	GtkWidget *button;

	g_return_val_if_fail (GTK_IS_DIALOG (dialog), NULL);
	g_return_val_if_fail (text != NULL, NULL);
	g_return_val_if_fail (stock_id != NULL, NULL);

	button = gtk_button_new_with_mnemonic (text);
	gtk_button_set_image (GTK_BUTTON (button),
			      gtk_image_new_from_stock (stock_id,
							GTK_ICON_SIZE_BUTTON));

	GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);

	gtk_widget_show (button);

	gtk_dialog_add_action_widget (dialog, button, response_id);

	return button;
}

static gboolean
replace_dialog (GtkWindow *parent,
		const gchar *message,
		const gchar *file_name)
{
	GtkWidget *message_dialog;
	gint ret;

	g_return_val_if_fail (file_name != NULL, FALSE);

	message_dialog = gtk_message_dialog_new (parent,
						 GTK_DIALOG_DESTROY_WITH_PARENT,
						 GTK_MESSAGE_QUESTION,
						 GTK_BUTTONS_NONE,
						 message,
						 file_name);
	/* Add cancel button */
	gtk_dialog_add_button (GTK_DIALOG (message_dialog),
			       GTK_STOCK_CANCEL,
			       GTK_RESPONSE_CANCEL);
	/* Add replace button */
	gsr_dialog_add_button (GTK_DIALOG (message_dialog), _("_Replace"),
			       GTK_STOCK_REFRESH,
			       GTK_RESPONSE_YES);

	gtk_dialog_set_default_response (GTK_DIALOG (message_dialog), GTK_RESPONSE_CANCEL);
	gtk_window_set_resizable (GTK_WINDOW (message_dialog), FALSE);
	ret = gtk_dialog_run (GTK_DIALOG (message_dialog));
	gtk_widget_destroy (GTK_WIDGET (message_dialog));

	return (ret == GTK_RESPONSE_YES);
}

static gboolean
replace_existing_file (GtkWindow *parent,
		      const gchar *file_name) 
{
	return replace_dialog (parent,
			       _("A file named \"%s\" already exists. \n"
			       "Do you want to replace it with the "
			       "one you are saving?"),
			       file_name);
}

static void
do_save_file (GSRWindow *window,
	      const char *_name)
{
	GSRWindowPrivate *priv;
        GMAudioProfile *profile;
	char *tmp, *src, *name;
	GnomeVFSURI *src_uri, *dst_uri;

	priv = window->priv;

        profile = gm_audio_profile_choose_get_active (priv->profile);

	if (g_str_has_suffix (_name, gm_audio_profile_get_extension (profile)))
		name = g_strdup (_name);
	else
		name = g_strdup_printf ("%s.%s", _name,
			       gm_audio_profile_get_extension (profile));
	if (g_file_test (name, G_FILE_TEST_EXISTS)) {
		if (!replace_existing_file (GTK_WINDOW (window), name))
			return;
	}

	tmp = g_strdup_printf ("file://%s", name);
	src = g_strdup_printf ("file://%s", priv->record_filename);
	src_uri = gnome_vfs_uri_new (src);
	dst_uri = gnome_vfs_uri_new (tmp);
	g_free (src);
	if (src_uri && dst_uri) {
		GnomeVFSResult result;
		result = gnome_vfs_xfer_uri (src_uri, dst_uri,
			GNOME_VFS_XFER_DEFAULT,
			GNOME_VFS_XFER_ERROR_MODE_ABORT,
			GNOME_VFS_XFER_OVERWRITE_MODE_REPLACE,
			NULL, NULL);
		if (result == GNOME_VFS_OK) {
			g_object_set (G_OBJECT (window), "location", name, NULL);
			priv->dirty = FALSE;
			window->priv->saved = TRUE;
			if ( GPOINTER_TO_INT (g_object_get_data (G_OBJECT (window), "save_quit")))
				gsr_window_close (window);
		} else {
			gchar *error_message;
 
			error_message = g_strdup_printf (_("Could not save the file \"%s\""), gnome_vfs_result_to_string (result));
			show_error_dialog (GTK_WINDOW (window), error_message);
			g_free (error_message);
		}
		gnome_vfs_uri_unref (src_uri);
		gnome_vfs_uri_unref (dst_uri);
	} else {
		gchar *error_message;

		error_message = g_strdup_printf (_("Could not save the file \"%s\""), name);
		show_error_dialog (GTK_WINDOW (window), error_message);
		g_free (error_message);
	}

	g_free (name);
	g_free (tmp);
}

static void
file_chooser_save_response_cb (GtkDialog *file_chooser,
			       int response_id,
			       GSRWindow *window)
{
	char *name;
	char *dirname;

	if (response_id != GTK_RESPONSE_OK)
		goto out;
	
	name = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (file_chooser));
	if (name == NULL)
		goto out;

	dirname = g_path_get_dirname (name);
	gconf_client_set_string (gconf_client, KEY_SAVE_DIR, dirname, NULL);
	g_free (dirname);
	
	do_save_file (window, name);
	g_free (name);

 out:	
	g_object_set_data (G_OBJECT (window), "save_quit", GPOINTER_TO_INT (NULL));
	gtk_widget_destroy (GTK_WIDGET (file_chooser));
}

static void
file_save_as_cb (GtkAction *action,
		 GSRWindow *window)
{
	GtkWidget *file_chooser;
	char *directory;

	file_chooser = gtk_file_chooser_dialog_new (_("Save file as"),
						    GTK_WINDOW (window),
						    GTK_FILE_CHOOSER_ACTION_SAVE,
						    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
						    GTK_STOCK_SAVE, GTK_RESPONSE_OK,
						    NULL);

	directory = gconf_client_get_string (gconf_client, KEY_SAVE_DIR, NULL);
	if (directory != NULL && *directory != 0) {
		gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (file_chooser), directory);
	}
	g_free (directory);

	if (window->priv->filename != NULL) {
		char *basename;
		gchar *filename, *filename_ext, *extension;
		gint length;
		GMAudioProfile *profile;

		basename = g_path_get_basename (window->priv->filename);
		length = strlen (basename);
		extension = strrchr (basename, '.');

		if (extension != NULL) {
			length = length - strlen (extension);
		}

		filename = g_strndup (basename,length);
		profile = gm_audio_profile_choose_get_active (window->priv->profile);
		g_free (window->priv->extension);

		window->priv->extension = g_strdup (gm_audio_profile_get_extension (profile));
		filename_ext = g_strdup_printf ("%s.%s", filename , window->priv->extension);
		gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (file_chooser),
						   filename_ext);
		g_free (filename);
		g_free (filename_ext);
		g_free (basename);
	}

	g_signal_connect (G_OBJECT (file_chooser), "response",
			  G_CALLBACK (file_chooser_save_response_cb), window);
	gtk_widget_show (file_chooser);
}

static void
file_save_cb (GtkAction *action,
	      GSRWindow *window)
{
	if (window->priv->filename == NULL ||
	    /* Translator comment: Untitled here implies a track without a
	     * name. See also the translation in gnome-recorder.c:94. Those
	     * two strings should match! If the track is unnamed, we will
	     * open the save-as dialog here, else we´ll use the given file
	     * to save to. */
	    g_strrstr (window->priv->filename, _("Untitled")) == 0) {
		file_save_as_cb (NULL, window);
	} else {
		do_save_file (window, window->priv->filename);
	}
}

static void
run_mixer_cb (GtkAction *action,
	       GSRWindow *window)
{
	char *mixer_path;
	char *argv[2] = {NULL, NULL};
	GError *error = NULL;
	gboolean ret;

	/* Open the mixer */
	mixer_path = g_find_program_in_path ("gnome-volume-control");
	if (mixer_path == NULL) {
		char *tmp;
		tmp = g_strdup_printf(_("%s is not installed in the path."), "gnome-volume-control");
		show_error_dialog (GTK_WINDOW (window), tmp);
		g_free(tmp);
		return;
	}

	argv[0] = mixer_path;
	ret = g_spawn_async (NULL, argv, NULL, 0, NULL, NULL, NULL, &error);
	if (ret == FALSE) {
		char *tmp;
		tmp = g_strdup_printf (_("There was an error starting %s: %s"),
				       mixer_path, error->message);
		show_error_dialog (GTK_WINDOW (window), tmp);
		g_free(tmp);
		g_error_free (error);
	}

	g_free (mixer_path);
}

gboolean
gsr_window_is_saved (GSRWindow *window)
{
	return window->priv->saved;
}

static void
handle_confirmation_response (GtkDialog *dialog, gint response_id, GSRWindow *window)
{
	switch (response_id) {
		case GTK_RESPONSE_YES:
			g_object_set_data (G_OBJECT (window), "save_quit", GINT_TO_POINTER (TRUE));
			file_save_as_cb (NULL, window);
				break;

		case GTK_RESPONSE_NO:
			gsr_window_close (window);
			break;

		case GTK_RESPONSE_CANCEL:
		default: 
			break;
	} 

	gtk_widget_destroy (GTK_WIDGET (dialog));
	return;
}

void 
close_confirmation_dialog (GSRWindow *window)
{
	GtkDialog *confirmation_dialog;
	char *msg;
	AtkObject *atk_obj;

	msg = g_strdup_printf (_("Save the changes to file \"%s\" before closing?"),
			       window->priv->record_filename);

	confirmation_dialog = gtk_message_dialog_new_with_markup (NULL,
								  GTK_DIALOG_MODAL,
						 		  GTK_MESSAGE_WARNING,
								  GTK_BUTTONS_NONE,
								  "<span weight=\"bold\" size=\"larger\">%s</span>",
								  msg);

	gtk_dialog_add_buttons (confirmation_dialog,
			 	_("Close _without Saving"), GTK_RESPONSE_NO,
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				GTK_STOCK_SAVE, GTK_RESPONSE_YES, NULL);

	gtk_window_set_title (GTK_WINDOW (confirmation_dialog), "");

	g_signal_connect (G_OBJECT (confirmation_dialog), "response",
			  G_CALLBACK (handle_confirmation_response), window);

	atk_obj = gtk_widget_get_accessible (GTK_WIDGET (confirmation_dialog));
	atk_object_set_name (atk_obj, _("Question"));

	gtk_widget_show_all (GTK_WIDGET (confirmation_dialog));
	g_free (msg);
}

static GtkWidget *
make_title_label (const char *text)
{
	GtkWidget *label;
	char *fulltext;
	
	fulltext = g_strdup_printf ("<span weight=\"bold\">%s</span>", text);
	label = gtk_label_new (fulltext);
	g_free (fulltext);

	gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.0);
	return label;
}

static GtkWidget *
make_info_label (const char *text)
{
	GtkWidget *label;

	label = gtk_label_new (text);
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_label_set_selectable (GTK_LABEL (label), TRUE);
	gtk_label_set_line_wrap (GTK_LABEL (label), GTK_WRAP_WORD);

	return label;
}

static void
pack_table_widget (GtkWidget *table,
		   GtkWidget *widget,
		   int left,
		   int top)
{
	gtk_table_attach (GTK_TABLE (table), widget,
			  left, left + 1, top, top + 1,
			  GTK_FILL, GTK_FILL, 0, 0);
}

struct _file_props {
	GtkWidget *dialog;

	GtkWidget *dirname;
	GtkWidget *filename;
	GtkWidget *size;
	GtkWidget *length;
	GtkWidget *samplerate;
	GtkWidget *channels;
	GtkWidget *bitrate;
};

static void
fill_in_information (GSRWindow *window,
		     struct _file_props *fp)
{
	char *text;
	char *name;
	struct stat buf;

	/* dirname */
	text = g_path_get_dirname (window->priv->filename);
	gtk_label_set_text (GTK_LABEL (fp->dirname), text);
	g_free (text);

	/* filename */
	name = g_path_get_basename (window->priv->filename);
	if (window->priv->dirty) {
		text = g_strdup_printf (_("%s (Has not been saved)"), name);
	} else {
		text = g_strdup (name);
	}
	gtk_label_set_text (GTK_LABEL (fp->filename), text);
	g_free (text);
	g_free (name);
	
	/* Size */
	if (stat (window->priv->working_file, &buf) == 0) {
		char *human;
		human = gnome_vfs_format_file_size_for_display (buf.st_size);

		text = g_strdup_printf (ngettext ("%s (%llu byte)", "%s (%llu bytes)", 
						   buf.st_size), human, buf.st_size);
		g_free (human);
	} else {
		text = g_strdup (_("Unknown size"));
	}
	gtk_label_set_text (GTK_LABEL (fp->size), text);
	g_free (text);

	/* FIXME: Set up and run our own pipeline 
	          till we can get the info */
	/* Length */
	if (window->priv->len_secs == 0) {
		text = g_strdup (_("Unknown"));
	} else {
		text = seconds_to_full_string (window->priv->len_secs);
	}
	gtk_label_set_text (GTK_LABEL (fp->length), text);
	g_free (text);

	/* sample rate */
	if (window->priv->samplerate == 0) {
		text = g_strdup (_("Unknown"));
	} else {
		text = g_strdup_printf (_("%.1f kHz"), (float) window->priv->samplerate / 1000);
	}
	gtk_label_set_text (GTK_LABEL (fp->samplerate), text);
	g_free (text);

	/* bit rate */
	if (window->priv->bitrate == 0) {
		text = g_strdup (_("Unknown"));
	} else {
		text = g_strdup_printf (_("%.0f kb/s"), (float) window->priv->bitrate / 1000);
	}
	gtk_label_set_text (GTK_LABEL (fp->bitrate), text);
	g_free (text);

	/* channels */
	switch (window->priv->n_channels) {
	case 0:
		text = g_strdup (_("Unknown"));
		break;
	case 1:
		text = g_strdup (_("1 (mono)"));
		break;
	case 2:
		text = g_strdup (_("2 (stereo)"));
		break;
	default:
		text = g_strdup_printf ("%d", window->priv->n_channels);
		break;
	}
	gtk_label_set_text (GTK_LABEL (fp->channels), text);
	g_free (text);
}

static void
dialog_closed_cb (GtkDialog *dialog,
		  guint response_id,
		  struct _file_props *fp)
{
	gtk_widget_destroy (fp->dialog);
	g_free (fp);
}

static void
file_properties_cb (GtkAction *action,
		    GSRWindow *window)
{
	GtkWidget *dialog, *vbox, *inner_vbox, *hbox, *table, *label;
	char *title, *shortname;
	struct _file_props *fp;
	shortname = g_path_get_basename (window->priv->filename);
	title = g_strdup_printf (_("%s Information"), shortname);
	g_free (shortname);

	dialog = gtk_dialog_new_with_buttons (title, GTK_WINDOW (window),
					      GTK_DIALOG_DESTROY_WITH_PARENT,
					      GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE, NULL);
	g_free (title);
	gtk_dialog_set_has_separator (GTK_DIALOG (dialog), FALSE);
	gtk_window_set_resizable (GTK_WINDOW (dialog), FALSE);
	gtk_container_set_border_width (GTK_CONTAINER (dialog), 5);
	gtk_box_set_spacing (GTK_BOX (GTK_DIALOG (dialog)->vbox), 2);
	fp = g_new (struct _file_props, 1);
	fp->dialog = dialog;

	g_signal_connect (G_OBJECT (dialog), "response",
			  G_CALLBACK (dialog_closed_cb), fp);

	vbox = gtk_vbox_new (FALSE, 18);
	gtk_container_set_border_width (GTK_CONTAINER (vbox), 5);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), vbox, TRUE, TRUE, 0);

	inner_vbox = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (vbox), inner_vbox, FALSE, FALSE,0);

	label = make_title_label (_("File Information"));
	gtk_box_pack_start (GTK_BOX (inner_vbox), label, FALSE, FALSE, 0);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (inner_vbox), hbox, TRUE, TRUE, 0);

	label = gtk_label_new ("    ");
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	/* File properties */	
	table = gtk_table_new (3, 2, FALSE);
	gtk_table_set_col_spacings (GTK_TABLE (table), 12);
	gtk_table_set_row_spacings (GTK_TABLE (table), 6);
	gtk_box_pack_start (GTK_BOX (hbox), table, TRUE, TRUE, 0);

	label = make_info_label (_("Folder:"));
	pack_table_widget (table, label, 0, 0);

	fp->dirname = make_info_label ("");
	pack_table_widget (table, fp->dirname, 1, 0);

	label = make_info_label (_("Filename:"));
	pack_table_widget (table, label, 0, 1);

	fp->filename = make_info_label ("");
	pack_table_widget (table, fp->filename, 1, 1);

	label = make_info_label (_("File size:"));
	pack_table_widget (table, label, 0, 2);

	fp->size = make_info_label ("");
	pack_table_widget (table, fp->size, 1, 2);

	inner_vbox = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (vbox), inner_vbox, FALSE, FALSE, 0);

	label = make_title_label (_("Audio Information"));
	gtk_box_pack_start (GTK_BOX (inner_vbox), label, FALSE, FALSE, 0);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (inner_vbox), hbox, TRUE, TRUE, 0);

	label = gtk_label_new ("    ");
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	/* Audio info */
	table = gtk_table_new (4, 2, FALSE);
	gtk_table_set_col_spacings (GTK_TABLE (table), 12);
	gtk_table_set_row_spacings (GTK_TABLE (table), 6);
	gtk_box_pack_start (GTK_BOX (hbox), table, TRUE, TRUE, 0);

	label = make_info_label (_("File duration:"));
	pack_table_widget (table, label, 0, 0);

	fp->length = make_info_label ("");
	pack_table_widget (table, fp->length, 1, 0);

	label = make_info_label (_("Number of channels:"));
	pack_table_widget (table, label, 0, 1);

	fp->channels = make_info_label ("");
	pack_table_widget (table, fp->channels, 1, 1);

	label = make_info_label (_("Sample rate:"));
	pack_table_widget (table, label, 0, 2);

	fp->samplerate = make_info_label ("");
	pack_table_widget (table, fp->samplerate, 1, 2);

	label = make_info_label (_("Bit rate:"));
	pack_table_widget (table, label, 0, 3);

	fp->bitrate = make_info_label ("");
	pack_table_widget (table, fp->bitrate, 1, 3);

	fill_in_information (window, fp);
	gtk_widget_show_all (dialog);
}

void
gsr_window_close (GSRWindow *window)
{
	gtk_widget_destroy (GTK_WIDGET (window));
}

static void
file_close_cb (GtkAction *action,
	       GSRWindow *window)
{
	if (! gsr_window_is_saved (window)) {
		close_confirmation_dialog (window);
	} else {
		gsr_window_close (window);
	}
}

static void
quit_cb (GtkAction *action,
	 GSRWindow *window)
{
	gsr_quit ();
}

static void
help_contents_cb (GtkAction *action,
	          GSRWindow *window)
{
	GError *error = NULL;

	gnome_help_display ("gnome-sound-recorder.xml", NULL, &error);

	if (error != NULL)
	{
		g_warning (error->message);

		g_error_free (error);
	}
}

static void
about_cb (GtkAction *action,
	  GSRWindow *window)
{
	const char * const authors[] = {"Iain Holmes <iain@prettypeople.org>", 
		"Ronald Bultje <rbultje@ronald.bitfreak.net>", 
		"Johan Dahlin  <johan@gnome.org>", 
		NULL};
	const char * const documenters[] = {"Sun Microsystems", NULL};
 
	gtk_show_about_dialog (GTK_WINDOW (window),
			       "name", _("Sound Recorder"),
			       "version", VERSION,
			       "copyright", "Copyright \xc2\xa9 2002 Iain Holmes",
			       "comments", _("A sound recorder for GNOME\n gnome-media@gnome.org"),
			       "authors", authors,
			       "documenters", documenters,
			       "logo-icon-name", "gnome-grecord",
			       NULL);
}

static void
play_cb (GtkAction *action,
	 GSRWindow *window)
{
	GSRWindowPrivate *priv = window->priv;

	if (priv->has_file == FALSE)
		return;

	if (priv->play) {
		gst_element_set_state (priv->play->pipeline, GST_STATE_NULL);
		g_object_unref (priv->play->pipeline);
	}
	priv->play = make_play_pipeline (window);

	g_object_set (G_OBJECT (window->priv->play->src),
		      "location", priv->filename,
		      NULL);

	if (priv->record && gst_element_get_state (priv->record->pipeline) == GST_STATE_PLAYING) {
		gst_element_set_state (priv->record->pipeline, GST_STATE_READY);
	}

	if (priv->play)
		gst_element_set_state (priv->play->pipeline, GST_STATE_PLAYING);
}

static void
cb_rec_eos (GstElement * element,
	    GSRWindow * window)
{
	GSRWindowPrivate *priv = window->priv;

	gst_element_set_state (priv->record->pipeline, GST_STATE_READY);

	g_free (priv->working_file);
	priv->working_file = g_strdup (priv->record_filename);

	g_free (priv->filename);
	priv->filename = g_strdup (priv->record_filename);

	priv->dirty = TRUE;
	priv->has_file = TRUE;
}

static void
stop_cb (GtkAction *action,
	 GSRWindow *window)
{
	GSRWindowPrivate *priv = window->priv;

	/* Work out whats playing */
	if (priv->play && gst_element_get_state (priv->play->pipeline) == GST_STATE_PLAYING) {
		gst_element_set_state (priv->play->pipeline, GST_STATE_READY);
	} else if (priv->record && gst_element_get_state (priv->record->pipeline) == GST_STATE_PLAYING) {
#if 0
		gst_element_set_state (priv->record->pipeline, GST_STATE_READY);

		g_free (priv->working_file);
		priv->working_file = g_strdup (priv->record_filename);

		g_free (priv->filename);
		priv->filename = g_strdup (priv->record_filename);

		priv->dirty = TRUE;
		priv->has_file = TRUE;
#endif
		g_signal_connect (priv->record->pipeline, "eos",
				  G_CALLBACK (cb_rec_eos), window);
		gst_pad_send_event (gst_element_get_pad (priv->record->src,
							 "src"),
				    gst_event_new (GST_EVENT_EOS));
	}
}

static void
record_cb (GtkAction *action,
	   GSRWindow *window)
{
	GSRWindowPrivate *priv = window->priv;

	if (priv->record) {
		gst_element_set_state (priv->record->pipeline, GST_STATE_NULL);
		g_object_unref (priv->record->pipeline);
	}
	priv->record = make_record_pipeline (window);
	if (!priv->record)
		return;

	g_object_set (G_OBJECT (priv->record->sink),
		      "location", priv->record_filename,
		      NULL);
	window->priv->len_secs = 0;
	window->priv->saved = FALSE;
	gst_element_set_state (priv->record->pipeline, GST_STATE_PLAYING);
}

static gboolean
get_length (GSRWindow *window)
{
	gint64 value;
	GstFormat format = GST_FORMAT_TIME;
	gboolean query_worked = FALSE;

	if (window->priv->play->sink != NULL) {
		query_worked = gst_element_query (window->priv->play->sink,
						  GST_QUERY_TOTAL, &format, &value);
	}

	if (query_worked) {
		char *len_str;

		window->priv->len_secs = value / GST_SECOND;

		len_str = seconds_to_full_string (window->priv->len_secs);
		gtk_label_set (GTK_LABEL (window->priv->length_label), len_str);
		
		g_free (len_str);

		return FALSE;
	} else {
		if (window->priv->get_length_attempts-- < 1) {
			/* Attempts to get length ran out. */
			gtk_label_set (GTK_LABEL (window->priv->length_label), _("Unknown"));
			return FALSE;
		}
	}

	return (gst_element_get_state (window->priv->play->pipeline) == GST_STATE_PLAYING);
}

static gboolean
seek_started (GtkRange *range,
	      GdkEventButton *event,
	      GSRWindow *window)
{
	g_return_val_if_fail (window->priv != NULL, FALSE);

	window->priv->seek_in_progress = TRUE;
	return FALSE;
}

static gboolean
seek_to (GtkRange *range,
	 GdkEventButton *gdkevent,
	 GSRWindow *window)
{
	double value = range->adjustment->value;
	gint64 time;
	GstEvent *event;
	GstElementState old_state;

	old_state = gst_element_get_state (window->priv->play->pipeline);
	if (old_state == GST_STATE_READY || old_state == GST_STATE_NULL) {
		return FALSE;
	}

	gst_element_set_state (window->priv->play->pipeline, GST_STATE_PAUSED);
	time = ((value / 100) * window->priv->len_secs) * GST_SECOND;

	event = gst_event_new_seek (GST_FORMAT_TIME | GST_SEEK_FLAG_FLUSH, time);
	gst_element_send_event (window->priv->play->sink, event);
	gst_element_set_state (window->priv->play->pipeline, old_state);
	window->priv->seek_in_progress = FALSE;

	return FALSE;
}

static gboolean
tick_callback (GSRWindow *window)
{
	int secs;
	gint64 value;
	gboolean query_worked = FALSE;
	GstFormat format = GST_FORMAT_TIME;

	if (gst_element_get_state (window->priv->play->pipeline) != GST_STATE_PLAYING) {
		/* This check stops us from doing an unnecessary query */
		return FALSE;
	}

#if 0
	if (window->priv->len_secs == 0) {
		/* Check if we've exhausted the length check yet */
		if (window->priv->get_length_attempts == 0) {
			return FALSE;
		} else {
			return TRUE;
		}
	}
#endif
	get_length (window);

	if (window->priv->seek_in_progress)
		return TRUE;

	query_worked = gst_element_query (window->priv->play->sink,
					  GST_QUERY_POSITION, 
					  &format, &value);
	if (query_worked && window->priv->len_secs != 0) {
		double percentage;
		secs = value / GST_SECOND;
		
		percentage = ((double) secs / (double) window->priv->len_secs) * 100.0;
		gtk_adjustment_set_value (GTK_RANGE (window->priv->scale)->adjustment, percentage + 0.5);

	}

	return (gst_element_get_state (window->priv->play->pipeline) == GST_STATE_PLAYING);
}

static gboolean
record_tick_callback (GSRWindow *window)
{
	int secs;
	gint64 value;
	gboolean query_worked = FALSE;
	GstFormat format = GST_FORMAT_TIME;

	if (gst_element_get_state (window->priv->record->pipeline) != GST_STATE_PLAYING) {
		/* This check stops us from doing an unnecessary query */
		return FALSE;
	}

	if (window->priv->seek_in_progress)
		return TRUE;

	query_worked = gst_element_query (window->priv->record->sink,
					  GST_QUERY_POSITION, 
					  &format, &value);
	if (query_worked) {
		gchar* len_str;
		secs = value / GST_SECOND;
		
		len_str = seconds_to_full_string (secs);
		window->priv->len_secs = secs;
		gtk_label_set (GTK_LABEL (window->priv->length_label), len_str);
		g_free (len_str);
	}

	return (gst_element_get_state (window->priv->record->pipeline) == GST_STATE_PLAYING);
}

static gboolean
play_iterate (GSRWindow *window)
{
	gboolean ret;
	ret =  gst_bin_iterate (GST_BIN (window->priv->play->pipeline));

	if (!ret)
		gst_element_set_state (window->priv->play->pipeline, GST_STATE_NULL);
	return ret;
}

static void
play_state_changed_cb (GstElement *element,
		       GstElementState old,
		       GstElementState state,
		       GSRWindow *window)
{
	switch (state) {
	case GST_STATE_PLAYING:
		g_idle_add ((GSourceFunc) play_iterate, window);
		window->priv->tick_id = g_timeout_add (200, (GSourceFunc) tick_callback, window);
#if 0
		if (window->priv->len_secs == 0) {
			window->priv->get_length_attempts = 16;
			g_timeout_add (200, (GSourceFunc) get_length, window);
		}
#endif		
		set_action_sensitive (window, "Stop", TRUE);
		set_action_sensitive (window, "Play", FALSE);
		set_action_sensitive (window, "Record", FALSE);
		set_action_sensitive (window, "FileSave", FALSE);
		set_action_sensitive (window, "FileSaveAs", FALSE);
		gtk_widget_set_sensitive (window->priv->scale, TRUE);

		gtk_statusbar_pop (GTK_STATUSBAR (window->priv->statusbar),
				   window->priv->status_message_cid);
		gtk_statusbar_push (GTK_STATUSBAR (window->priv->statusbar),
				    window->priv->status_message_cid,
				    _("Playing..."));

		set_ebusy_timeout (window, NULL);
		break;

	case GST_STATE_READY:
		gtk_adjustment_set_value (GTK_RANGE (window->priv->scale)->adjustment, 0.0);
		gtk_widget_set_sensitive (window->priv->scale, FALSE);
	case GST_STATE_PAUSED:
		set_action_sensitive (window, "Stop", FALSE);
		set_action_sensitive (window, "Play", TRUE);
		set_action_sensitive (window, "Record", TRUE);
		set_action_sensitive (window, "FileSave", window->priv->dirty ? TRUE : FALSE);
		set_action_sensitive (window, "FileSaveAs", TRUE);

		gtk_statusbar_pop (GTK_STATUSBAR (window->priv->statusbar),
				   window->priv->status_message_cid);
		gtk_statusbar_push (GTK_STATUSBAR (window->priv->statusbar),
				    window->priv->status_message_cid,
				    _("Ready"));
		break;
	default:
		break;
	}
}

static void
pipeline_deep_notify_cb (GstElement *element,
			 GstElement *orig,
			 GParamSpec *param,
			 GSRWindow *window)
{
	GSRWindowPrivate *priv;
	GObject *obj;
	const char *pname;

	obj = G_OBJECT (orig);
	priv = window->priv;
	
	pname = g_param_spec_get_name (param);
	if (strstr (pname, "channels")) {
		g_object_get (obj, "channels", &priv->n_channels, NULL);
	} else if (strstr (pname, "samplerate")) {
		g_object_get (obj,"samplerate", &priv->samplerate, NULL);
	} else if (strstr (pname, "bitrate")) {
		g_object_get (obj, "bitrate", &priv->bitrate, NULL);
	}
}

/* callback for when the recording profile has been changed */
static void
profile_changed_cb (GObject *object, GSRWindow *window)
{
	GMAudioProfile *profile;
	gchar *id;

	g_return_if_fail (GTK_IS_COMBO_BOX (object));
	profile = gm_audio_profile_choose_get_active (GTK_WIDGET (object));
  
	id = g_strdup (gm_audio_profile_get_id (profile));
	gconf_client_set_string (gconf_client, KEY_LAST_PROFILE_ID, id, NULL);
	g_free (id);
}

static GSRWindowPipeline *
make_play_pipeline (GSRWindow *window)
{
	GSRWindowPipeline *obj;
	GstElement *pipeline;
	guint32 id;
	GstElement *spider, *conv, *scale;

	pipeline = gst_pipeline_new ("play-pipeline");
	g_signal_connect (pipeline, "deep-notify",
			  G_CALLBACK (pipeline_deep_notify_cb), window);
	g_signal_connect (pipeline, "error",
			  G_CALLBACK (pipeline_error_cb), window);
	id = g_signal_connect (pipeline, "state-change",
			       G_CALLBACK (play_state_changed_cb), window);

	obj = g_new (GSRWindowPipeline, 1);
	obj->pipeline = pipeline;
	obj->state_change_id = id;
	
	obj->src = gst_element_factory_make ("filesrc", "src");
	if (!obj->src)
	{
		g_error ("Could not find \"filesrc\" GStreamer element");
		return NULL;
	}
	
 	spider = gst_element_factory_make ("spider", "spider");
	if (!spider) {
		g_error ("Could not find \"spider\" GStreamer element");
		return NULL;
	}

	conv = gst_element_factory_make ("audioconvert", "conf");
	scale = gst_element_factory_make ("audioscale", "scale");
	obj->sink = gst_gconf_get_default_audio_sink ();
	if (!obj->sink)
	{
		g_error ("Could not find default audio output element");
		return NULL;
	}

	gst_bin_add_many (GST_BIN (pipeline),
			  obj->src, spider, conv, scale, obj->sink, NULL);
	gst_element_link_many (obj->src, spider, conv, scale, obj->sink, NULL);

	return obj;
}

extern int sample_count;

static gboolean
record_start (gpointer user_data) 
{
	GSRWindow *window = GSR_WINDOW (user_data);
	gchar *name;

	window->priv->get_length_attempts = 16;
	g_timeout_add (200, (GSourceFunc) record_tick_callback, window);

	set_action_sensitive (window, "Stop", TRUE);
	set_action_sensitive (window, "Play", FALSE);
	set_action_sensitive (window, "Record", FALSE);
	set_action_sensitive (window, "FileSave", FALSE);
	set_action_sensitive (window, "FileSaveAs", FALSE);
	gtk_widget_set_sensitive (window->priv->scale, FALSE);

	gtk_statusbar_pop (GTK_STATUSBAR (window->priv->statusbar),
			   window->priv->status_message_cid);
	gtk_statusbar_push (GTK_STATUSBAR (window->priv->statusbar),
			    window->priv->status_message_cid,
			    _("Recording..."));

	window->priv->record_id = 0;

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
	gtk_window_set_title (GTK_WINDOW(window), name);

	g_free (name);

	return FALSE;
}

static void
record_state_changed_cb (GstElement *element,
			 GstElementState old,
			 GstElementState state,
			 GSRWindow *window)
{
	switch (state) {
	case GST_STATE_PLAYING:
		window->priv->record_id = g_idle_add (record_start, window);
		gtk_widget_set_sensitive (window->priv->profile, FALSE);
		break;
	case GST_STATE_READY:
		gtk_adjustment_set_value (GTK_RANGE (window->priv->scale)->adjustment, 0.0);
		gtk_widget_set_sensitive (window->priv->scale, FALSE);
		gtk_widget_set_sensitive (window->priv->profile, TRUE);
		/* fall through */
	case GST_STATE_PAUSED:
		set_action_sensitive (window, "Stop", FALSE);
		set_action_sensitive (window, "Play", TRUE);
		set_action_sensitive (window, "Record", TRUE);
		set_action_sensitive (window, "FileSave", window->priv->dirty ? TRUE : FALSE);
		set_action_sensitive (window, "FileSaveAs", TRUE);
		gtk_widget_set_sensitive (window->priv->scale, FALSE);

		gtk_statusbar_pop (GTK_STATUSBAR (window->priv->statusbar),
				   window->priv->status_message_cid);
		gtk_statusbar_push (GTK_STATUSBAR (window->priv->statusbar),
				    window->priv->status_message_cid,
				    _("Ready"));
		break;
	default:
		break;
	}
}

static GSRWindowPipeline *
make_record_pipeline (GSRWindow *window)
{
	GSRWindowPipeline *pipeline;
	gint32 id;
	GMAudioProfile *profile;
	gchar *pipeline_desc, *source;
	GstElement *esource = NULL, *encoder, *manager;
	
	pipeline = g_new (GSRWindowPipeline, 1);
	pipeline->pipeline = gst_thread_new ("record-pipeline");
	id = g_signal_connect (G_OBJECT (pipeline->pipeline),
			       "state-change",
			       G_CALLBACK (record_state_changed_cb),
			       window);
	pipeline->state_change_id = id;
	
	g_signal_connect (G_OBJECT (pipeline->pipeline), "error",
			  G_CALLBACK (pipeline_error_cb), window);
	
        profile = gm_audio_profile_choose_get_active (window->priv->profile);
	source = gst_gconf_get_string ("default/audiosrc");
	if (source) {
		esource = gst_gconf_render_bin_from_description (source);
	}
	if (!esource) {
		show_error_dialog (NULL, _("There is no default GStreamer "
				   "audio input plugin set - please install "
				   "the GStreamer-GConf schemas or set one "
				   "manually"));
		return NULL;
	}
	pipeline->src = manager =
		gst_element_factory_make ("manager", "manager");
	gst_bin_add (GST_BIN (manager), esource);
	gst_bin_add (GST_BIN (pipeline->pipeline), manager);

	pipeline_desc = g_strdup_printf ("audioconvert ! %s",
					 gm_audio_profile_get_pipeline (profile));
	g_free (source);
	
	encoder = gst_gconf_render_bin_from_description (pipeline_desc);
	g_free (pipeline_desc);
	if (!encoder) {
		show_error_dialog (NULL, _("Failed to create GStreamer "
				   "encoder plugins - check your encoding "
				   "setup"));
		return NULL;
	}
	gst_bin_add (GST_BIN (pipeline->pipeline), encoder);
	
	pipeline->sink = gst_element_factory_make ("filesink", "sink");
	if (!pipeline->sink)
	{
		show_error_dialog (NULL, _("Could not find \"filesink\" GStreamer "
				   "plugin - broken GStreamer install"));
		return NULL;
	}
	gst_bin_add (GST_BIN (pipeline->pipeline), pipeline->sink);
	
	if (!gst_element_link (manager, encoder) ||
	    !gst_element_link (encoder, pipeline->sink))
	{
		show_error_dialog (NULL, _("Failed to link encoder plugin "
				   "with file output plugin - you probably "
				   "selected an invalid encoder"));
		return NULL;
	}
	
	return pipeline;
}

static char *
calculate_format_value (GtkScale *scale,
			double value,
			GSRWindow *window)
{
	int seconds;

	if (window->priv->record && gst_element_get_state (window->priv->record->pipeline) == GST_STATE_PLAYING) {
		seconds = value;
		return seconds_to_string (seconds);
	} else {
		seconds = window->priv->len_secs * (value / 100);
		return seconds_to_string (seconds);
	}
}
	
static const GtkActionEntry menu_entries[] =
{
	/* File menu.  */
	{ "File", NULL, N_("_File") },
	{ "FileNew", GTK_STOCK_NEW, NULL, NULL,
	  N_("Create a new sample"), G_CALLBACK (file_new_cb) },
	{ "FileOpen", GTK_STOCK_OPEN, N_("_Open..."), NULL,
	  N_("Open a file"), G_CALLBACK (file_open_cb) },
	{ "FileSave", GTK_STOCK_SAVE, NULL, NULL,
	  N_("Save the current file"), G_CALLBACK (file_save_cb) },
	{ "FileSaveAs", GTK_STOCK_SAVE_AS, N_("Save _As..."), "<shift><control>S",
	  N_("Save the current file with a different name"), G_CALLBACK (file_save_as_cb) },
	{ "RunMixer", GTK_STOCK_EXECUTE, N_("Open Volu_me Control"), NULL,
	  N_("Open the audio mixer"), G_CALLBACK (run_mixer_cb) },
	{ "FileProperties", GTK_STOCK_PROPERTIES, NULL, "<control>I",
	  N_("Show information about the current file"), G_CALLBACK (file_properties_cb) },
	{ "FileClose", GTK_STOCK_CLOSE, NULL, NULL,
	  N_("Close the current file"), G_CALLBACK (file_close_cb) },
	{ "Quit", GTK_STOCK_QUIT, NULL, NULL,
	  N_("Quit the program"), G_CALLBACK (quit_cb) },

	/* Control menu */
	{ "Control", NULL, N_("_Control") },
	{ "Record", GTK_STOCK_MEDIA_RECORD, NULL, "<control>R",
	  N_("Record sound"), G_CALLBACK (record_cb) },
	{ "Play", GTK_STOCK_MEDIA_PLAY, NULL, "<control>P",
	  N_("Play sound"), G_CALLBACK (play_cb) },
	{ "Stop", GTK_STOCK_MEDIA_STOP, NULL, "<control>X",
	  N_("Stop sound"), G_CALLBACK (stop_cb) },

	/* Help menu */
	{ "Help", NULL, N_("_Help") },
	{"HelpContents", GTK_STOCK_HELP, NULL, NULL,
	 N_("Open the manual"), G_CALLBACK (help_contents_cb) },
	{ "About", GTK_STOCK_ABOUT, NULL, NULL,
	 N_("About this application"), G_CALLBACK (about_cb) }
};

static void
menu_item_select_cb (GtkMenuItem *proxy,
                     GSRWindow *window)
{
	GtkAction *action;
	char *message;

	action = g_object_get_data (G_OBJECT (proxy),  "gtk-action");
	g_return_if_fail (action != NULL);

	g_object_get (G_OBJECT (action), "tooltip", &message, NULL);
	if (message) {
		gtk_statusbar_push (GTK_STATUSBAR (window->priv->statusbar),
				    window->priv->tip_message_cid, message);
		g_free (message);
	}
}

static void
menu_item_deselect_cb (GtkMenuItem *proxy,
                       GSRWindow *window)
{
	gtk_statusbar_pop (GTK_STATUSBAR (window->priv->statusbar),
			   window->priv->tip_message_cid);
}
 
static void
connect_proxy_cb (GtkUIManager *manager,
                  GtkAction *action,
                  GtkWidget *proxy,
                  GSRWindow *window)
{
	if (GTK_IS_MENU_ITEM (proxy)) {
		g_signal_connect (proxy, "select",
				  G_CALLBACK (menu_item_select_cb), window);
		g_signal_connect (proxy, "deselect",
				  G_CALLBACK (menu_item_deselect_cb), window);
	}
}

static void
disconnect_proxy_cb (GtkUIManager *manager,
                     GtkAction *action,
                     GtkWidget *proxy,
                     GSRWindow *window)
{
	if (GTK_IS_MENU_ITEM (proxy)) {
		g_signal_handlers_disconnect_by_func
			(proxy, G_CALLBACK (menu_item_select_cb), window);
		g_signal_handlers_disconnect_by_func
			(proxy, G_CALLBACK (menu_item_deselect_cb), window);
	}
}

static void
gsr_window_init (GSRWindow *window)
{
	GSRWindowPrivate *priv;
	GError *error = NULL;
	GtkWidget *main_vbox;
	GtkWidget *menubar;
	GtkWidget *file_menu;
	GtkWidget *submenu;
	GtkWidget *rec_menu;
	GtkWidget *toolbar;
	GtkWidget *content_vbox;
	GtkWidget *hbox;
	GtkWidget *label;
	GtkWidget *table;
	gchar *id;
	GtkAction *action;

	window->priv = GSR_WINDOW_GET_PRIVATE (window);
	priv = window->priv;

	/* treat gconf client as a singleton */
	if (gconf_client == NULL)
		gconf_client = gconf_client_get_default ();

	main_vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (window), main_vbox);
	priv->main_vbox = main_vbox;
	gtk_widget_show (main_vbox);

	/* menu & toolbar */
	priv->ui_manager = gtk_ui_manager_new ();

	gtk_window_add_accel_group (GTK_WINDOW (window),
				    gtk_ui_manager_get_accel_group (priv->ui_manager));

	gtk_ui_manager_add_ui_from_file (priv->ui_manager, GSR_UIDIR "ui.xml", &error);
	if (error != NULL)
	{
		show_error_dialog (GTK_WINDOW (window),
			_("Could not load ui.xml. The program may be not properly installed"));
		g_error_free (error);
		exit (1);
	}

	/* show tooltips in the statusbar */
	g_signal_connect (priv->ui_manager, "connect_proxy",
			  G_CALLBACK (connect_proxy_cb), window);
	g_signal_connect (priv->ui_manager, "disconnect_proxy",
			 G_CALLBACK (disconnect_proxy_cb), window);

	priv->action_group = gtk_action_group_new ("GSRWindowActions");
	gtk_action_group_set_translation_domain (priv->action_group, NULL);
	gtk_action_group_add_actions (priv->action_group,
				      menu_entries,
				      G_N_ELEMENTS (menu_entries),
				      window);

	gtk_ui_manager_insert_action_group (priv->ui_manager, priv->action_group, 0);

	/* set short labels to use in the toolbar */
	action = gtk_action_group_get_action (priv->action_group, "FileOpen");
	g_object_set (action, "short_label", _("Open"), NULL);
	action = gtk_action_group_get_action (priv->action_group, "FileSave");
	g_object_set (action, "short_label", _("Save"), NULL);
	action = gtk_action_group_get_action (priv->action_group, "FileSaveAs");
	g_object_set (action, "short_label", _("Save As"), NULL);

	set_action_sensitive (window, "FileSave", FALSE);
	set_action_sensitive (window, "FileSaveAs", FALSE);
	set_action_sensitive (window, "Play", FALSE);
	set_action_sensitive (window, "Stop", FALSE);

	menubar = gtk_ui_manager_get_widget (priv->ui_manager, "/MenuBar");
	gtk_box_pack_start (GTK_BOX (main_vbox), menubar, FALSE, FALSE, 0);
	gtk_widget_show (menubar);

	toolbar = gtk_ui_manager_get_widget (priv->ui_manager, "/ToolBar");
	gtk_box_pack_start (GTK_BOX (main_vbox), toolbar, FALSE, FALSE, 0);
	gtk_widget_show (toolbar);

	/* recent files */
	file_menu = gtk_ui_manager_get_widget (priv->ui_manager,
					      "/MenuBar/FileMenu");
	submenu = gtk_menu_item_get_submenu (GTK_MENU_ITEM (file_menu));
	rec_menu = gtk_ui_manager_get_widget (priv->ui_manager,
					      "/MenuBar/FileMenu/FileRecentMenu");
	priv->recent_view = egg_recent_view_gtk_new (submenu, rec_menu);
	egg_recent_view_gtk_show_icons (EGG_RECENT_VIEW_GTK (priv->recent_view),
					FALSE);
	egg_recent_view_gtk_set_trailing_sep (priv->recent_view, TRUE);
	egg_recent_view_set_model (EGG_RECENT_VIEW (priv->recent_view), recent_model); 
	g_signal_connect (priv->recent_view, "activate",
			  G_CALLBACK (file_open_recent_cb), window);

	/* window content: hscale, labels, etc */
	content_vbox = gtk_vbox_new (FALSE, 6);
	gtk_container_set_border_width (GTK_CONTAINER (content_vbox), 6);
	gtk_box_pack_start (GTK_BOX (main_vbox), content_vbox, TRUE, TRUE, 0);
	gtk_widget_show (content_vbox);

	priv->scale = gtk_hscale_new (GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, 100, 1, 1, 0)));
	priv->seek_in_progress = FALSE;
	g_signal_connect (priv->scale, "format-value",
			  G_CALLBACK (calculate_format_value), window);
	g_signal_connect (priv->scale, "button_press_event",
			  G_CALLBACK (seek_started), window);
	g_signal_connect (priv->scale, "button_release_event",
			  G_CALLBACK (seek_to), window);

	gtk_scale_set_value_pos (GTK_SCALE (window->priv->scale), GTK_POS_BOTTOM);
	/* We can't seek until we find out the length */
	gtk_widget_set_sensitive (window->priv->scale, FALSE);
	gtk_box_pack_start (GTK_BOX (content_vbox), priv->scale, TRUE, TRUE, 6);
	gtk_widget_show (window->priv->scale);

	hbox = gtk_hbox_new (FALSE, 12);
	gtk_box_pack_start (GTK_BOX (content_vbox), hbox, FALSE, FALSE, 0);

	label = gtk_label_new (_("Record as:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	priv->profile = gm_audio_profile_choose_new ();
	gtk_box_pack_start (GTK_BOX (hbox), window->priv->profile, TRUE, TRUE, 0);
	gtk_widget_show (window->priv->profile);

	id = gconf_client_get_string (gconf_client, KEY_LAST_PROFILE_ID, NULL);
	if (id) {
		gm_audio_profile_choose_set_active (window->priv->profile, id);
		g_free (id);
	}

        g_signal_connect (priv->profile, "changed",
                          G_CALLBACK (profile_changed_cb), window);

	label = make_title_label (_("File Information"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_box_pack_start (GTK_BOX (content_vbox), label, FALSE, FALSE, 0);

	hbox = gtk_hbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (content_vbox), hbox, FALSE, FALSE, 0);

	label = gtk_label_new ("    ");
	gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	table = gtk_table_new (2, 2, FALSE);
	gtk_table_set_col_spacings (GTK_TABLE (table), 12);
	gtk_table_set_row_spacings (GTK_TABLE (table), 6);
	gtk_box_pack_start (GTK_BOX (hbox), table, TRUE, TRUE, 0);

	label = gtk_label_new (_("Filename:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
	gtk_table_attach (GTK_TABLE (table), label,
			  0, 1, 0, 1,
			  GTK_FILL, GTK_FILL, 0, 0);

	priv->name_label = gtk_label_new (_("<none>"));
	gtk_label_set_selectable (GTK_LABEL (priv->name_label), TRUE);
	gtk_label_set_line_wrap (GTK_LABEL (priv->name_label), GTK_WRAP_WORD);
	gtk_misc_set_alignment (GTK_MISC (priv->name_label), 0, 0.5);
	gtk_table_attach (GTK_TABLE (table), priv->name_label,
			  1, 2, 0, 1,
			  GTK_FILL, GTK_FILL,
			  0, 0);

	label = gtk_label_new (_("Length:"));
	gtk_misc_set_alignment (GTK_MISC (label), 0, 0.5);
	gtk_table_attach (GTK_TABLE (table), label,
			  0, 1, 1, 2,
			  GTK_FILL, 0, 0, 0);
	
	priv->length_label = gtk_label_new ("");
	gtk_label_set_selectable (GTK_LABEL (priv->length_label), TRUE);
	gtk_misc_set_alignment (GTK_MISC (priv->length_label), 0, 0.5);
	gtk_table_attach (GTK_TABLE (table), priv->length_label,
			  1, 2, 1, 2,
			  GTK_FILL, GTK_FILL,
			  0, 0);

	/* statusbar */
	priv->statusbar = gtk_statusbar_new ();
	gtk_box_pack_end (GTK_BOX (main_vbox), priv->statusbar, FALSE, FALSE, 0);
	gtk_widget_show (priv->statusbar);
	priv->status_message_cid = gtk_statusbar_get_context_id
		(GTK_STATUSBAR (priv->statusbar), "status_message");
	priv->tip_message_cid = gtk_statusbar_get_context_id
		(GTK_STATUSBAR (priv->statusbar), "tip_message");

	gtk_statusbar_push (GTK_STATUSBAR (priv->statusbar),
			    priv->status_message_cid,
			    _("Ready"));

	gtk_widget_show_all (main_vbox);

	/* Make the pipelines */
	priv->play = NULL;
	priv->record = NULL; 

	priv->len_secs = 0;
	priv->get_length_attempts = 16;
	priv->dirty = FALSE;
	priv->gstenc_id = 0;
}

static void
gsr_window_set_property (GObject      *object,
			 guint         prop_id,
			 const GValue *value,
			 GParamSpec   *pspec)
{
	GSRWindow *window;
	GSRWindowPrivate *priv;
	struct stat buf;
	char *title, *short_name;

	window = GSR_WINDOW (object);
	priv = window->priv;

	switch (prop_id) {
	case PROP_LOCATION:
		if (priv->filename != NULL) {
			if (strcmp (g_value_get_string (value), priv->filename) == 0) {
				return;
			}
		}

		g_free (priv->filename);
		g_free (priv->working_file);

		priv->filename = g_value_dup_string (value);
		priv->working_file = g_strdup (priv->filename);
		priv->len_secs = 0;

		short_name = g_path_get_basename (priv->filename);
		if (stat (priv->filename, &buf) == 0) {
			window->priv->has_file = TRUE;
		} else {
			window->priv->has_file = FALSE;
		}

		if (priv->name_label != NULL) {
			gtk_label_set (GTK_LABEL (priv->name_label), short_name);
		}

		if (recent_model) {
			gsr_add_recent (priv->filename);
		}

		title = g_strdup_printf ("%s - Sound Recorder", short_name);
		gtk_window_set_title (GTK_WINDOW (window), title);
		g_free (title);
		g_free (short_name);

		set_action_sensitive (window, "Play", window->priv->has_file ? TRUE : FALSE);
		set_action_sensitive (window, "Stop", FALSE);
		set_action_sensitive (window, "Record", TRUE);
		/* set_action_sensitive (window, "FileSave", FALSE); */
		set_action_sensitive (window, "FileSaveAs", window->priv->has_file ? TRUE : FALSE);
		break;
	default:
		break;
	}
}

static void
gsr_window_get_property (GObject    *object,
			 guint       prop_id,
			 GValue     *value,
			 GParamSpec *pspec)
{
	switch (prop_id) {
	case PROP_LOCATION:
		g_value_set_string (value, GSR_WINDOW (object)->priv->filename);
		break;

	default:
		break;
	}
}

static void
gsr_window_finalize (GObject *object)
{
	GSRWindow *window;
	GSRWindowPrivate *priv;

	window = GSR_WINDOW (object);
	priv = window->priv;

	if (priv == NULL) {
		return;
	}

	if (priv->ui_manager) {
		g_object_unref (priv->ui_manager);
		priv->ui_manager = NULL;
	}

	if (priv->action_group) {
		g_object_unref (priv->action_group);
		priv->action_group = NULL;
	}

	if (priv->tick_id > 0) { 
		g_source_remove (priv->tick_id);
	}

	if (priv->record_id > 0) {
		g_source_remove (priv->record_id);
	}

	g_idle_remove_by_data (window);

	if (priv->play != NULL) {
		shutdown_pipeline (priv->play);
		g_free (priv->play);
	}

	if (priv->record != NULL) {
		shutdown_pipeline (priv->record);
		g_free (priv->record);
	}

	unlink (priv->record_filename);
	g_free (priv->record_filename);

	g_free (priv->working_file);
	g_free (priv->filename);

	G_OBJECT_CLASS (parent_class)->finalize (object);

	window->priv = NULL;
}

static void
gsr_window_class_init (GSRWindowClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = gsr_window_finalize;
	object_class->set_property = gsr_window_set_property;
	object_class->get_property = gsr_window_get_property;

	parent_class = g_type_class_peek_parent (klass);

	g_object_class_install_property (object_class,
					 PROP_LOCATION,
					 g_param_spec_string ("location",
							      "Location",
							      "",
	/* Translator comment: default trackname is 'untitled', which
	 * has as effect that the user cannot save to this file. The
	 * 'save' action will open the save-as dialog instead to give
	 * a proper filename. See gnome-record.c:94. */
							      _("Untitled"),
							      G_PARAM_READWRITE));

	g_type_class_add_private (object_class, sizeof (GSRWindowPrivate));
}

GType
gsr_window_get_type (void)
{
	static GType type = 0;

	if (G_UNLIKELY (type == 0)) {
		GTypeInfo info = {
			sizeof (GSRWindowClass),
			NULL, NULL,
			(GClassInitFunc) gsr_window_class_init,
			NULL, NULL,
			sizeof (GSRWindow), 0,
			(GInstanceInitFunc) gsr_window_init
		};

		type = g_type_register_static (GTK_TYPE_WINDOW,
					       "GSRWindow",
					       &info, 0);
	}

	return type;
}

GtkWidget *
gsr_window_new (const char *filename)
{
	GSRWindow *window;
	struct stat buf;
	char *short_name;

        /* filename has been changed to be without extension */
	window = g_object_new (GSR_TYPE_WINDOW, 
			       "location", filename,
			       NULL);
        /* FIXME: check extension too */
	window->priv->filename = g_strdup (filename);
	if (stat (filename, &buf) == 0) {
		window->priv->has_file = TRUE;
	} else {
		window->priv->has_file = FALSE;
	}

	window->priv->record_filename = g_strdup_printf ("%s/gsr-record-%s-%d.XXXXXX",
							 g_get_tmp_dir(), filename, getpid ());
	window->priv->record_fd = g_mkstemp (window->priv->record_filename);
	close (window->priv->record_fd);

	if (window->priv->has_file == FALSE) {
		g_free (window->priv->working_file);
		window->priv->working_file = g_strdup (window->priv->record_filename);
	} else {
		g_free (window->priv->working_file);
		window->priv->working_file = g_strdup (filename);
	}

	window->priv->saved = TRUE;

	gtk_window_set_default_size (GTK_WINDOW (window), 512, 200);

	return GTK_WIDGET (window);
}

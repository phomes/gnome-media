/* GNOME Volume Control
 * Copyright (C) 2003-2004 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *
 * preferences.c: preferences screen
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
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <string.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gconf/gconf-client.h>

#include "element.h"
#include "preferences.h"
#include "keys.h"
#include "track.h"
#include "misc.h"

enum {
  COL_ACTIVE,
  COL_LABEL,
  COL_TRACK,
  COL_TYPE,
  COL_PAGE,
  NUM_COLS
};

G_DEFINE_TYPE (GnomeVolumeControlPreferences, gnome_volume_control_preferences, GTK_TYPE_DIALOG)

static void	gnome_volume_control_preferences_class_init	(GnomeVolumeControlPreferencesClass *klass);
static void	gnome_volume_control_preferences_init	(GnomeVolumeControlPreferences *prefs);
static void	gnome_volume_control_preferences_dispose (GObject *object);
static void	gnome_volume_control_preferences_response (GtkDialog *dialog,
							   gint       response_id);

static void	set_gconf_track_active	(GConfClient *client, GstMixer *mixer, 
					 GstMixerTrack *track, gboolean active);


static void	cb_toggle		(GtkCellRendererToggle *cell,
					 gchar                 *path_str,
					 gpointer               data);
static void	cb_activated		(GtkTreeView *view, GtkTreePath *path,
					 GtkTreeViewColumn *col, gpointer userdata);
static void	cb_gconf		(GConfClient     *client,
					 guint            connection_id,
					 GConfEntry      *entry,
					 gpointer         userdata);


static void
gnome_volume_control_preferences_class_init (GnomeVolumeControlPreferencesClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkDialogClass *gtkdialog_class = (GtkDialogClass *) klass;

  gobject_class->dispose = gnome_volume_control_preferences_dispose;
  gtkdialog_class->response = gnome_volume_control_preferences_response;
}

/*
 * Mixer tracks are sorted by their types.
 */
static gint
sort_by_page_num (GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer user_data)
{
	gint a_type, b_type;

	gtk_tree_model_get (model, a, COL_PAGE, &a_type, -1);
	gtk_tree_model_get (model, b, COL_PAGE, &b_type, -1);

	return (a_type - b_type);
}

static void
gnome_volume_control_preferences_init (GnomeVolumeControlPreferences *prefs)
{
  GtkWidget *box, *label, *view;
  GtkListStore *store;
  GtkTreeSelection *sel;
  GtkTreeViewColumn *col;
  GtkCellRenderer *render;

  prefs->client = NULL;
  prefs->client_cnxn = 0;
  prefs->mixer = NULL;

  /* make window look cute */
  gtk_window_set_title (GTK_WINDOW (prefs), _("Volume Control Preferences"));
  gtk_dialog_set_has_separator (GTK_DIALOG (prefs), FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (prefs), 5);
  gtk_box_set_spacing (GTK_BOX (GTK_DIALOG(prefs)->vbox), 2);
  gtk_dialog_add_buttons (GTK_DIALOG (prefs),
			  GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE,
			  /* help goes here (future) */
			  NULL);

  /* add a treeview for all the properties */
  box = gtk_vbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (box), 5);

  label = gtk_label_new_with_mnemonic (_("_Select mixers to be visible:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  store = gtk_list_store_new (NUM_COLS, G_TYPE_BOOLEAN,
			      G_TYPE_STRING, G_TYPE_POINTER, G_TYPE_STRING,
			      G_TYPE_INT);
  gtk_tree_sortable_set_default_sort_func (GTK_TREE_SORTABLE (store), sort_by_page_num, NULL, NULL);
  gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (store), GTK_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID, GTK_SORT_ASCENDING);
  prefs->treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (prefs->treeview), FALSE);
  gtk_label_set_mnemonic_widget (GTK_LABEL(label), GTK_WIDGET (prefs->treeview));

  /* viewport for lots of tracks */
  view = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (view),
				  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (view),
				       GTK_SHADOW_IN);
  gtk_widget_set_size_request (view, -1, 250);

  gtk_container_add (GTK_CONTAINER (view), prefs->treeview);
  gtk_box_pack_start (GTK_BOX (box), view, TRUE, TRUE, 0);

  gtk_widget_show (prefs->treeview);
  gtk_widget_show (view);

  /* treeview internals */
  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (prefs->treeview));
  gtk_tree_selection_set_mode (sel, GTK_SELECTION_SINGLE);

  render = gtk_cell_renderer_toggle_new ();
  g_signal_connect (render, "toggled",
		    G_CALLBACK (cb_toggle), prefs);
  g_signal_connect (prefs->treeview, "row-activated",
		    G_CALLBACK (cb_activated), prefs);
  col = gtk_tree_view_column_new_with_attributes ("Active", render,
						  "active", COL_ACTIVE,
						  NULL);
  gtk_tree_view_column_set_clickable (col, TRUE);
  gtk_tree_view_append_column (GTK_TREE_VIEW (prefs->treeview), col);

  render = gtk_cell_renderer_text_new ();
  col = gtk_tree_view_column_new_with_attributes ("Track name", render,
						  "text", COL_LABEL,
						  NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (prefs->treeview), col);

  render = gtk_cell_renderer_text_new ();
  col = gtk_tree_view_column_new_with_attributes ("Type", render,
                          "text", COL_TYPE,
						  NULL);
  gtk_tree_view_append_column (GTK_TREE_VIEW (prefs->treeview), col);

  gtk_tree_view_set_search_column (GTK_TREE_VIEW (prefs->treeview), COL_LABEL);

  /* and show */
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (prefs)->vbox), box,
		      TRUE, TRUE, 0);
  gtk_widget_show (box);
}

GtkWidget *
gnome_volume_control_preferences_new (GstElement  *element,
				      GConfClient *client)
{
  GnomeVolumeControlPreferences *prefs;

  g_return_val_if_fail (GST_IS_MIXER (element), NULL);

  /* element */
  prefs = g_object_new (GNOME_VOLUME_CONTROL_TYPE_PREFERENCES, NULL);
  prefs->client = g_object_ref (G_OBJECT (client));

  gnome_volume_control_preferences_change (prefs, element);

  /* gconf */
  prefs->client_cnxn = gconf_client_notify_add (prefs->client, 
						GNOME_VOLUME_CONTROL_KEY_DIR,
						cb_gconf, prefs, NULL, NULL);

  return GTK_WIDGET (prefs);
}

static void
gnome_volume_control_preferences_dispose (GObject *object)
{
  GnomeVolumeControlPreferences *prefs;

  prefs = GNOME_VOLUME_CONTROL_PREFERENCES (object);

  if (prefs->client) {
    gconf_client_notify_remove (prefs->client, prefs->client_cnxn);
    g_object_unref (G_OBJECT (prefs->client));
    prefs->client = NULL;
  }

  if (prefs->mixer) {
    gst_object_unref (GST_OBJECT (prefs->mixer));
    prefs->mixer = NULL;
  }

  G_OBJECT_CLASS (gnome_volume_control_preferences_parent_class)->dispose (object);
}

static void
gnome_volume_control_preferences_response (GtkDialog *dialog,
					   gint       response_id)
{
  switch (response_id) {
    case GTK_RESPONSE_CLOSE:
      gtk_widget_destroy (GTK_WIDGET (dialog));
      break;

    default:
      break;
  }

  if (((GtkDialogClass *) gnome_volume_control_preferences_parent_class)->response)
    ((GtkDialogClass *) gnome_volume_control_preferences_parent_class)->response (dialog, response_id);
}

/*
 * Hide non-alphanumeric characters, for saving in gconf.
 */

gchar *
get_gconf_key (GstMixer *mixer, GstMixerTrack *track)
{
  const gchar *dev;
  gchar *res;
  gint i, pos;
  gchar *label = NULL;

  g_return_val_if_fail(mixer != NULL, NULL);

  dev = g_object_get_data (G_OBJECT (mixer),
			   "gnome-volume-control-name");
  if (track != NULL) {
    label = g_strdup (track->label);
  } else {
    label = g_strdup ("");
  }

  pos = strlen (GNOME_VOLUME_CONTROL_KEY_DIR) + 1;
  res = g_new (gchar, pos + strlen (dev) + 1 + strlen (label) + 1);
  strcpy (res, GNOME_VOLUME_CONTROL_KEY_DIR "/");

  for (i = 0; dev[i] != '\0'; i++) {
    if (g_ascii_isalnum (dev[i]))
      res[pos++] = dev[i];
  }
  res[pos] = '/';
  for (i = 0; label[i] != '\0'; i++) {
    if (g_ascii_isalnum (label[i]))
      res[pos++] = label[i];
  }
  res[pos] = '\0';

  g_free (label);
  return res;
}

/*
 * Change the element. Basically recreates this object internally.
 */

void
gnome_volume_control_preferences_change (GnomeVolumeControlPreferences *prefs,
					 GstElement *element)
{
  GstMixer *mixer;
  GtkTreeIter iter;
  GtkListStore *store;
  const GList *item;
  gint pgnum;

  g_return_if_fail (GST_IS_MIXER (element));
  mixer = GST_MIXER (element);

  store = GTK_LIST_STORE (gtk_tree_view_get_model (GTK_TREE_VIEW (prefs->treeview)));

  /* remove old */
  while (gtk_tree_model_get_iter_first (GTK_TREE_MODEL (store), &iter)) {
    gtk_list_store_remove (store, &iter);
  }

  /* take/put reference */
  gst_object_replace ((GstObject **) &prefs->mixer, GST_OBJECT (element));

  /* add all tracks */
  gnome_volume_control_element_whitelist (mixer, NULL);
  for (item = gst_mixer_list_tracks (mixer);
       item != NULL; item = item->next) {
    GstMixerTrack *track = item->data;
    gchar *key = get_gconf_key (mixer, track);
    GConfValue *value;
    gboolean active = gnome_volume_control_element_whitelist (mixer, track);

    if ((value = gconf_client_get (prefs->client, key, NULL)) != NULL &&
        value->type == GCONF_VALUE_BOOL) {
      active = gconf_value_get_bool (value);
    }
    g_free (key);

    pgnum = get_page_num (mixer, track);
    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter,
			COL_ACTIVE, active,
			COL_LABEL, track->label,
			COL_TRACK, track,
			COL_TYPE, get_page_description (pgnum),
			COL_PAGE, pgnum,
			-1);
  }
}

/*
 * Callback if something is toggled.
 */

static void
set_gconf_track_active(GConfClient *client, GstMixer *mixer, 
		       GstMixerTrack *track, gboolean active)
{
  gchar *key;

  key = get_gconf_key (mixer, track);
  gconf_client_set_bool (client, key, active, NULL);
  g_free (key);
}

static void	
cb_gconf(GConfClient *client, guint connection_id, 
	 GConfEntry *entry, gpointer userdata)
{
  GnomeVolumeControlPreferences *prefs;
  GConfValue *value;
  GtkTreeIter iter;
  GtkTreeModel *model;
  gchar *keybase;
  gboolean active, valid;
  GstMixerTrack *track;

  prefs = GNOME_VOLUME_CONTROL_PREFERENCES (userdata);
  model = gtk_tree_view_get_model (GTK_TREE_VIEW(prefs->treeview));
  keybase = get_gconf_key (prefs->mixer, NULL);

  if (g_str_equal (gconf_entry_get_key (entry), keybase) &&
      (value = gconf_entry_get_value (entry)) != NULL &&
      (value->type == GCONF_VALUE_BOOL)) {
    active = gconf_value_get_bool (value); 
    valid = gtk_tree_model_get_iter_first(model, &iter);

    while (valid == TRUE) {
      gtk_tree_model_get (model, &iter,
			  COL_TRACK, &track,
			  -1);
      if (g_str_equal (track->label, gconf_entry_get_key (entry) + strlen (keybase))) {
	gtk_list_store_set( GTK_LIST_STORE(model), &iter, COL_ACTIVE, active, -1);
	break ;
      }
      valid = gtk_tree_model_iter_next(model, &iter);
    }
  }
}

static void
cb_activated(GtkTreeView *view, GtkTreePath *path,
	     GtkTreeViewColumn *col, gpointer userdata)

{
  GtkTreeModel *model;
  GtkTreeIter iter;
  gboolean active;
  GstMixerTrack *track;
  GnomeVolumeControlPreferences *prefs;

  prefs = GNOME_VOLUME_CONTROL_PREFERENCES (userdata);
  model = gtk_tree_view_get_model(view);

  if (gtk_tree_model_get_iter(model, &iter, path)) {
    gtk_tree_model_get(model, &iter, 
		       COL_ACTIVE, &active, 
		       COL_TRACK, &track,
		       -1);

    active = !active;

    gtk_list_store_set( GTK_LIST_STORE(model), &iter, COL_ACTIVE, active, -1);
    set_gconf_track_active(prefs->client, prefs->mixer, track, active);
  }
}

static void
cb_toggle (GtkCellRendererToggle *cell,
	   gchar                 *path_str,
	   gpointer               data)
{
  GnomeVolumeControlPreferences *prefs = data;
  GtkTreeModel *model = gtk_tree_view_get_model (GTK_TREE_VIEW (prefs->treeview));
  GtkTreePath *path = gtk_tree_path_new_from_string (path_str);
  GtkTreeIter iter;
  gboolean active;
  GstMixerTrack *track;

  gtk_tree_model_get_iter (model, &iter, path);
  gtk_tree_model_get (model, &iter,
		      COL_ACTIVE, &active,
		      COL_TRACK, &track,
		      -1);

  active = !active;

  gtk_list_store_set (GTK_LIST_STORE (model), &iter,
		      COL_ACTIVE, active,
		      -1);
  gtk_tree_path_free (path);

  set_gconf_track_active(prefs->client, prefs->mixer, track, active);
}

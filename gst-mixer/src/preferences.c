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

#include <gnome.h>
#include <gtk/gtk.h>
#include <gst/mixer/mixer.h>

#include "element.h"
#include "preferences.h"
#include "keys.h"
#include "track.h"

enum {
  COL_ACTIVE,
  COL_LABEL,
  COL_TRACK,
  NUM_COLS
};

static void	gnome_volume_control_preferences_class_init	(GnomeVolumeControlPreferencesClass *klass);
static void	gnome_volume_control_preferences_init	(GnomeVolumeControlPreferences *prefs);
static void	gnome_volume_control_preferences_dispose (GObject *object);
static void	gnome_volume_control_preferences_response (GtkDialog *dialog,
							   gint       response_id);

static void	cb_toggle		(GtkCellRendererToggle *cell,
					 gchar                 *path_str,
					 gpointer               data);

static GtkNotebookClass *parent_class = NULL;

GType
gnome_volume_control_preferences_get_type (void)
{
  static GType gnome_volume_control_preferences_type = 0;

  if (!gnome_volume_control_preferences_type) {
    static const GTypeInfo gnome_volume_control_preferences_info = {
      sizeof (GnomeVolumeControlPreferencesClass),
      NULL,
      NULL,
      (GClassInitFunc) gnome_volume_control_preferences_class_init,
      NULL,
      NULL,
      sizeof (GnomeVolumeControlPreferences),
      0,
      (GInstanceInitFunc) gnome_volume_control_preferences_init,
      NULL
    };

    gnome_volume_control_preferences_type =
	g_type_register_static (GTK_TYPE_DIALOG, 
				"GnomeVolumeControlPreferences",
				&gnome_volume_control_preferences_info, 0);
  }

  return gnome_volume_control_preferences_type;
}

static void
gnome_volume_control_preferences_class_init (GnomeVolumeControlPreferencesClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkDialogClass *gtkdialog_class = (GtkDialogClass *) klass;

  parent_class = g_type_class_ref (GTK_TYPE_DIALOG);

  gobject_class->dispose = gnome_volume_control_preferences_dispose;
  gtkdialog_class->response = gnome_volume_control_preferences_response;
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

  label = gtk_label_new_with_mnemonic (_("_Select tracks to be visible:"));
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
  gtk_box_pack_start (GTK_BOX (box), label, FALSE, FALSE, 0);
  gtk_widget_show (label);

  store = gtk_list_store_new (NUM_COLS, G_TYPE_BOOLEAN,
			      G_TYPE_STRING, G_TYPE_POINTER);
  prefs->treeview = gtk_tree_view_new_with_model (GTK_TREE_MODEL (store));
  gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (prefs->treeview), FALSE);
  gtk_label_set_mnemonic_widget (GTK_LABEL(label), GTK_WIDGET (prefs->treeview));

  /* viewport for lots of tracks */
  view = gtk_scrolled_window_new (NULL, NULL);
  gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (view),
				  GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (view),
				       GTK_SHADOW_IN);
  gtk_widget_set_usize (view, -1, 150);

  gtk_container_add (GTK_CONTAINER (view), prefs->treeview);
  gtk_box_pack_start (GTK_BOX (box), view, TRUE, TRUE, 0);

  gtk_widget_show (prefs->treeview);
  gtk_widget_show (view);

  /* treeview internals */
  sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (prefs->treeview));
  gtk_tree_selection_set_mode (sel, GTK_SELECTION_NONE);

  render = gtk_cell_renderer_toggle_new ();
  g_signal_connect (render, "toggled",
		    G_CALLBACK (cb_toggle), prefs);
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

  /* FIXME:
   * - update preferences dialog if user touches gconf directly.
   */

  return GTK_WIDGET (prefs);
}

static void
gnome_volume_control_preferences_dispose (GObject *object)
{
  GnomeVolumeControlPreferences *prefs = GNOME_VOLUME_CONTROL_PREFERENCES (object);

  if (prefs->client) {
    g_object_unref (G_OBJECT (prefs->client));
    prefs->client = NULL;
  }

  if (prefs->mixer) {
    gst_object_unref (GST_OBJECT (prefs->mixer));
    prefs->mixer = NULL;
  }

  G_OBJECT_CLASS (parent_class)->dispose (object);
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

  if (((GtkDialogClass *) parent_class)->response)
    ((GtkDialogClass *) parent_class)->response (dialog, response_id);
}

/*
 * Hide non-alphanumeric characters, for saving in gconf.
 */

gchar *
get_gconf_key (GstMixer *mixer,
	       gchar    *track_label)
{
  gchar *res;
  const gchar *dev = g_object_get_data (G_OBJECT (mixer),
					"gnome-volume-control-name");
  gint i, pos;

  pos = strlen (GNOME_VOLUME_CONTROL_KEY_DIR) + 1;
  res = g_new (gchar, pos + strlen (dev) + 1 + strlen (track_label) + 1);
  strcpy (res, GNOME_VOLUME_CONTROL_KEY_DIR "/");

  for (i = 0; dev[i] != '\0'; i++) {
    if (g_ascii_isalnum (dev[i]))
      res[pos++] = dev[i];
  }
  res[pos] = '/';
  for (i = 0; track_label[i] != '\0'; i++) {
    if (g_ascii_isalnum (track_label[i]))
      res[pos++] = track_label[i];
  }
  res[pos] = '\0';

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
  gvc_whitelist list[] = whitelist_init_list;

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
  for (item = gst_mixer_list_tracks (mixer);
       item != NULL; item = item->next) {
    GstMixerTrack *track = item->data;
    gchar *key = get_gconf_key (mixer, track->label);
    GConfValue *value;
    gboolean active = gnome_volume_control_element_whitelist (track, list);

    if ((value = gconf_client_get (prefs->client, key, NULL)) != NULL &&
        value->type == GCONF_VALUE_BOOL) {
      active = gconf_value_get_bool (value);
    }
    g_free (key);

    gtk_list_store_append (store, &iter);
    gtk_list_store_set (store, &iter,
			COL_ACTIVE, active,
			COL_LABEL, track->label,
			COL_TRACK, track,
			-1);
  }
}

/*
 * Callback if something is toggled.
 */

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
  GConfValue *value;
  gchar *key;
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

  key = get_gconf_key (prefs->mixer, track->label);
  value = gconf_value_new (GCONF_VALUE_BOOL);
  gconf_value_set_bool (value, active);
  gconf_client_set (prefs->client, key, value, NULL);
  gconf_value_free (value);
  g_free (key);
}

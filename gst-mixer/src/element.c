/* GNOME Volume Control
 * Copyright (C) 2003-2004 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *
 * element.c: widget representation of a single mixer element
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
#include "keys.h"
#include "preferences.h"
#include "track.h"

static void	gnome_volume_control_element_class_init	(GnomeVolumeControlElementClass *klass);
static void	gnome_volume_control_element_init	(GnomeVolumeControlElement *el);
static void	gnome_volume_control_element_dispose	(GObject *object);

static void	cb_gconf			(GConfClient     *client,
						 guint            connection_id,
						 GConfEntry      *entry,
						 gpointer         data);

static GtkNotebookClass *parent_class = NULL;

GType
gnome_volume_control_element_get_type (void)
{
  static GType gnome_volume_control_element_type = 0;

  if (!gnome_volume_control_element_type) {
    static const GTypeInfo gnome_volume_control_element_info = {
      sizeof (GnomeVolumeControlElementClass),
      NULL,
      NULL,
      (GClassInitFunc) gnome_volume_control_element_class_init,
      NULL,
      NULL,
      sizeof (GnomeVolumeControlElement),
      0,
      (GInstanceInitFunc) gnome_volume_control_element_init,
      NULL
    };

    gnome_volume_control_element_type =
	g_type_register_static (GTK_TYPE_NOTEBOOK, 
				"GnomeVolumeControlElement",
				&gnome_volume_control_element_info, 0);
  }

  return gnome_volume_control_element_type;
}

static void
gnome_volume_control_element_class_init (GnomeVolumeControlElementClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_ref (GTK_TYPE_NOTEBOOK);

  gobject_class->dispose = gnome_volume_control_element_dispose;
}

static void
gnome_volume_control_element_init (GnomeVolumeControlElement *el)
{
  el->client = NULL;
  el->mixer = NULL;
  el->appbar = NULL;
}

GtkWidget *
gnome_volume_control_element_new (GstElement  *element,
				  GConfClient *client,
				  GnomeAppBar *appbar)
{
  GnomeVolumeControlElement *el;

  g_return_val_if_fail (GST_IS_MIXER (element), NULL);

  /* element */
  el = g_object_new (GNOME_VOLUME_CONTROL_TYPE_ELEMENT, NULL);
  el->client = g_object_ref (G_OBJECT (client));
  el->appbar = appbar;

  gconf_client_add_dir (el->client, GNOME_VOLUME_CONTROL_KEY_DIR,
			GCONF_CLIENT_PRELOAD_RECURSIVE, NULL);
  gconf_client_notify_add (el->client, GNOME_VOLUME_CONTROL_KEY_DIR,
			   cb_gconf, el, NULL, NULL);

  gnome_volume_control_element_change (el, element);

  return GTK_WIDGET (el);
}

static void
gnome_volume_control_element_dispose (GObject *object)
{
  GnomeVolumeControlElement *el = GNOME_VOLUME_CONTROL_ELEMENT (object);

  if (el->client) {
    g_object_unref (G_OBJECT (el->client));
    el->client = NULL;
  }

  if (el->mixer) {
    gst_object_unref (GST_OBJECT (el->mixer));
    el->mixer = NULL;
  }

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

/*
 * Change the element. Basically recreates this object internally.
 */

void
gnome_volume_control_element_change (GnomeVolumeControlElement *el,
				     GstElement *element)
{
  struct {
    gchar *label;
    GtkWidget *page, *old_sep, *new_sep;
    gboolean use;
    gint pos, height, width;
    GnomeVolumeControlTrack * (* get_track_widget) (GtkTable      *table,
						    gint           tab_pos,
						    GstMixer      *mixer,
						    GstMixerTrack *track,
						    GtkWidget     *left_sep,
						    GtkWidget     *right_sep,
						    GnomeAppBar   *appbar);
  } content[3] = {
    { _("Playback"), NULL, NULL, NULL, FALSE, 0, 5, 1,
      gnome_volume_control_track_add_playback },
    { _("Capture"),  NULL, NULL, NULL, FALSE, 0, 5, 1,
      gnome_volume_control_track_add_capture },
    { _("Options"),  NULL, NULL, NULL, FALSE, 0, 1, 3,
      gnome_volume_control_track_add_option }
  };
  gint i;
  const GList *item;
  GstMixer *mixer;

  g_return_if_fail (GST_IS_MIXER (element));
  mixer = GST_MIXER (element);

  /* remove old pages */
  while (gtk_notebook_get_n_pages (GTK_NOTEBOOK (el)) > 0) {
    gtk_notebook_remove_page (GTK_NOTEBOOK (el), 0);
  }

  /* take/put reference */
  if (el->mixer) {
    for (item = gst_mixer_list_tracks (el->mixer);
	 item != NULL; item = item->next) {
      GstMixerTrack *track = item->data;
      GnomeVolumeControlTrack *trkw;

      trkw = g_object_get_data (G_OBJECT (track),
				"gnome-volume-control-trkw");
      gnome_volume_control_track_free (trkw);
    }
  }
  gst_object_replace ((GstObject **) &el->mixer, GST_OBJECT (element));

  /* content pages */
  for (i = 0; i < 3; i++) {
    content[i].page = gtk_table_new (content[i].width, content[i].height, FALSE);
    gtk_container_set_border_width (GTK_CONTAINER (content[i].page), 6);
    if (i == 2)
      gtk_table_set_row_spacings (GTK_TABLE (content[i].page), 6);
    gtk_table_set_col_spacings (GTK_TABLE (content[i].page), 6);
  }

  /* show */
  for (item = gst_mixer_list_tracks (el->mixer);
       item != NULL; item = item->next) {
    GstMixerTrack *track = item->data;
    GtkWidget *trackw;
    GnomeVolumeControlTrack *trkw;
    gchar *key;
    const GConfValue *value;
    gboolean active = TRUE;

    if (GST_IS_MIXER_OPTIONS (track))
      i = 2;
    else if (track->flags & GST_MIXER_TRACK_INPUT)
      i = 1;
    else
      i = 0;

    /* FIXME:
     * - do not create separator if there is no more track
     *     _of this type_. We currently destroy it at the
     *     end, so it's not critical, but not nice either.
     */
    if (i == 2) {
      content[i].new_sep = gtk_hseparator_new ();
    } else {
      content[i].new_sep = gtk_vseparator_new ();
    }

    /* visible? */
    key = get_gconf_key (el->mixer, track->label);
    if ((value = gconf_client_get (el->client, key, NULL)) != NULL &&
        value->type == GCONF_VALUE_BOOL) {
      active = gconf_value_get_bool (value);
    }
    g_free (key);

    /* Show left separator if we're not the first track */
    if (active && content[i].use)
      gtk_widget_show (content[i].old_sep);

    /* widget */
    trkw = content[i].get_track_widget (GTK_TABLE (content[i].page),
					content[i].pos++, el->mixer, track,
					content[i].old_sep, content[i].new_sep,
					el->appbar);
    gnome_volume_control_track_show (trkw, active);

    g_object_set_data (G_OBJECT (track),
		       "gnome-volume-control-trkw", trkw);

    /* separator */
    if (item->next != NULL) {
      if (i == 2) {
        gtk_table_attach (GTK_TABLE (content[i].page), content[i].new_sep,
			  0, 3, content[i].pos, content[i].pos + 1,
			  GTK_EXPAND | GTK_FILL, 0, 0, 0);
      } else {
        gtk_table_attach (GTK_TABLE (content[i].page), content[i].new_sep,
			  content[i].pos, content[i].pos + 1, 0, 6,
			  0, GTK_EXPAND | GTK_FILL, 0, 0);
      }
      content[i].pos++;
    }

    content[i].old_sep = content[i].new_sep;
    content[i].use = TRUE;
  }

  /* show */
  for (i = 0; i < 3; i++) {
    GtkWidget *label, *view, *viewport;
    GtkAdjustment *hadjustment, *vadjustment;

    /* no empty pages */
    if (!content[i].use) {
      gtk_widget_destroy (content[i].page);
      continue;
    }

    /* don't show last separator */
    gtk_widget_destroy (content[i].new_sep);

    /* viewport for lots of tracks */
    view = gtk_scrolled_window_new (NULL, NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (view),
				    i == 2 ? GTK_POLICY_NEVER :
					     GTK_POLICY_AUTOMATIC,
				    i == 2 ? GTK_POLICY_AUTOMATIC :
					     GTK_POLICY_NEVER);

    hadjustment = gtk_scrolled_window_get_hadjustment (GTK_SCROLLED_WINDOW (view));
    vadjustment = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (view));
    viewport = gtk_viewport_new (hadjustment, vadjustment);
    gtk_viewport_set_shadow_type (GTK_VIEWPORT (viewport), GTK_SHADOW_NONE);

    gtk_container_add (GTK_CONTAINER (viewport), content[i].page);
    gtk_container_add (GTK_CONTAINER (view), viewport);

    label = gtk_label_new (content[i].label);
    gtk_notebook_append_page (GTK_NOTEBOOK (el), view, label);
    gtk_widget_show (content[i].page);
    gtk_widget_show (viewport);
    gtk_widget_show (view);
    gtk_widget_show (label);
  }
}

/*
 * GConf callback.
 */

static void
cb_gconf (GConfClient *client,
	  guint        connection_id,
	  GConfEntry  *entry,
	  gpointer     data)
{
  GnomeVolumeControlElement *el = GNOME_VOLUME_CONTROL_ELEMENT (data);
  gchar *keybase = get_gconf_key (el->mixer, "");

  if (!strncmp (gconf_entry_get_key (entry),
		keybase, strlen (keybase))) {
    const GList *item;

    for (item = gst_mixer_list_tracks (el->mixer);
	 item != NULL; item = item->next) {
      GstMixerTrack *track = item->data;
      GnomeVolumeControlTrack *trkw =
	g_object_get_data (G_OBJECT (track), "gnome-volume-control-trkw");
      gchar *key = get_gconf_key (el->mixer, track->label);

      if (!strcmp (gconf_entry_get_key (entry), key)) {
        GConfValue *value = gconf_entry_get_value (entry);

        if (value->type == GCONF_VALUE_BOOL) {
          gboolean active = gconf_value_get_bool (value),
		   first[3] = { TRUE, TRUE, TRUE };
          gint n;

          gnome_volume_control_track_show (trkw, active);

          /* separators */
          for (item = gst_mixer_list_tracks (el->mixer);
	       item != NULL; item = item->next) {
            GstMixerTrack *track = item->data;
            GnomeVolumeControlTrack *trkw =
	      g_object_get_data (G_OBJECT (track), "gnome-volume-control-trkw");

            if (GST_IS_MIXER_OPTIONS (track))
              n = 2;
            else if (track->flags & GST_MIXER_TRACK_INPUT)
              n = 1;
            else
              n = 0;

            if (trkw->visible && !first[n]) {
              g_assert (trkw->left_separator);
              gtk_widget_show (trkw->left_separator);
            } else if (trkw->left_separator) {
              gtk_widget_hide (trkw->left_separator);
            }

            if (trkw->visible && first[n])
              first[n] = FALSE;
          }
          break;
        }
      }

      g_free (key);
    }
  }
  g_free (keybase);
}

/* GNOME Volume Control
 * Copyright (C) 2003-2004 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *
 * track.c: layout of a single mixer track
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
#include <string.h>

#include "button.h"
#include "stock.h"
#include "track.h"
#include "volume.h"

static const struct {
  gchar *label,
	*pixmap;
} pix[] = {
  { "cd",         GTK_STOCK_CDROM                       },
  { "line",       GNOME_STOCK_LINE_IN                   },
  { "mic",        GNOME_STOCK_MIC                       },
  { "mix",        GNOME_VOLUME_CONTROL_STOCK_MIXER      },
  { "pcm",        GNOME_VOLUME_CONTROL_STOCK_TONE       },
  { "headphone",  GNOME_VOLUME_CONTROL_STOCK_HEADPHONES },
  { "phone",      GNOME_VOLUME_CONTROL_STOCK_PHONE      },
  { "speaker",    GNOME_STOCK_VOLUME                    },
  { "video",      GNOME_VOLUME_CONTROL_STOCK_VIDEO      },
  { "volume",     GNOME_VOLUME_CONTROL_STOCK_TONE       },
  { "master",     GNOME_VOLUME_CONTROL_STOCK_TONE       },
  { "3d",         GNOME_VOLUME_CONTROL_STOCK_3DSOUND    },
  { NULL, NULL }
};

/*
 * UI responses.
 */

static void
cb_mute_toggled (GnomeVolumeControlButton *button,
		 gpointer         data)
{
  GnomeVolumeControlTrack *ctrl = data;

  gst_mixer_set_mute (ctrl->mixer, ctrl->track,
		      !gnome_volume_control_button_get_active (button));
  gnome_volume_control_volume_sync (GNOME_VOLUME_CONTROL_VOLUME (ctrl->sliderbox));
}

static void
cb_record_toggled (GnomeVolumeControlButton *button,
		   gpointer         data)
{
  GnomeVolumeControlTrack *ctrl = data;

  gst_mixer_set_record (ctrl->mixer, ctrl->track,
		        gnome_volume_control_button_get_active (button));
}

/* Tells us whether toggling a switch should change the corresponding
 * GstMixerTrack's MUTE or RECORD flag.
 */
static gboolean
should_toggle_record_switch (const GstMixerTrack *track)
{
  return GST_MIXER_TRACK_HAS_FLAG (track, GST_MIXER_TRACK_INPUT);
}
 

static void
cb_toggle_changed (GtkToggleButton *button,
		   gpointer         data)
{
  GnomeVolumeControlTrack *ctrl = data;

  if (should_toggle_record_switch (ctrl->track)) {
    gst_mixer_set_record (ctrl->mixer, ctrl->track,
      gtk_toggle_button_get_active (button));
  } else {
    gst_mixer_set_mute (ctrl->mixer, ctrl->track,
      !gtk_toggle_button_get_active (button));
  }
}

static void
cb_option_changed (GtkComboBox *box,
		   gpointer     data)
{
  GnomeVolumeControlTrack *ctrl = data;
  GtkTreeIter iter;
  GtkTreeModel *model;
  gchar *opt;

  if (gtk_combo_box_get_active_iter (box, &iter)) {
    model = gtk_combo_box_get_model (box);
    gtk_tree_model_get (model, &iter, 0, &opt, -1);
    gst_mixer_set_option (ctrl->mixer, GST_MIXER_OPTIONS (ctrl->track), opt);
    g_free (opt);
  }
}

/*
 * Timeout to check for changes in mixer outside ourselves.
 */

static gboolean
cb_check (gpointer data)
{
  GnomeVolumeControlTrack *trkw = data;

  /* trigger an update of the mixer state */
  if (! GST_IS_MIXER_OPTIONS (trkw->track)) {
    gint *dummy = g_new (gint, 1);
    gst_mixer_get_volume (trkw->mixer, GST_MIXER_TRACK (trkw->track), dummy);
    g_free (dummy);
  }
  
  gboolean mute = GST_MIXER_TRACK_HAS_FLAG (trkw->track,
				GST_MIXER_TRACK_MUTE) ? TRUE : FALSE,
           record = GST_MIXER_TRACK_HAS_FLAG (trkw->track,
				GST_MIXER_TRACK_RECORD) ? TRUE : FALSE;
  gboolean vol_is_zero = FALSE, slider_is_zero = FALSE;

  if (trkw->sliderbox) {
    gnome_volume_control_volume_ask (
      GNOME_VOLUME_CONTROL_VOLUME (trkw->sliderbox),
      &vol_is_zero, &slider_is_zero);
  }
  if (!slider_is_zero && vol_is_zero)
    mute = TRUE;

  if (trkw->mute) {
    if (gnome_volume_control_button_get_active (trkw->mute) == mute) {
      gnome_volume_control_button_set_active (trkw->mute, !mute);
    }
  }

  if (trkw->record) {
    if (gnome_volume_control_button_get_active (trkw->record) != record) {
      gnome_volume_control_button_set_active (trkw->record, record);
    }
  }

  if (trkw->toggle) {
    if (should_toggle_record_switch (trkw->track)) {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (trkw->toggle),
        GST_MIXER_TRACK_HAS_FLAG (trkw->track, GST_MIXER_TRACK_RECORD));
    } else {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (trkw->toggle),
        !GST_MIXER_TRACK_HAS_FLAG (trkw->track, GST_MIXER_TRACK_MUTE));
    }
  }

  /* FIXME:
   * - options.
   */

  return TRUE;
}

/*
 * Actual UI code.
 */

static GnomeVolumeControlTrack *
gnome_volume_control_track_add_title (GtkTable *table,
				      gint      tab_pos,
				      GtkOrientation or,
				      GstMixer *mixer,
				      GstMixerTrack *track,
				      GtkWidget *l_sep,
				      GtkWidget *r_sep)
{
  GnomeVolumeControlTrack *ctrl;
  gchar *str = NULL;
  gboolean found = FALSE;
  gint i;

  /* start */
  ctrl = g_new0 (GnomeVolumeControlTrack, 1);
  ctrl->mixer = mixer;
  g_object_ref (G_OBJECT (track));
  ctrl->track = track;
  ctrl->left_separator = l_sep;
  ctrl->right_separator = r_sep;
  ctrl->visible = TRUE;
  ctrl->table = table;
  ctrl->pos = tab_pos;
  ctrl->id = g_timeout_add (200, cb_check, ctrl);

  /* image (optional) */
  for (i = 0; !found && pix[i].label != NULL; i++) {
    /* we dup the string to make the comparison case-insensitive */
    gchar *label_l = g_strdup (track->label);
    gint pos;

    /* make case insensitive */
    for (pos = 0; label_l[pos] != '\0'; pos++)
      label_l[pos] = g_ascii_tolower (label_l[pos]);

    if (g_strrstr (label_l, pix[i].label) != NULL) {
      str = pix[i].pixmap;
      found = TRUE;
    }

    g_free (label_l);
  }

  if (str != NULL) {
    if ((ctrl->image = gtk_image_new_from_stock (str, GTK_ICON_SIZE_MENU)) != NULL) {
      gtk_misc_set_alignment (GTK_MISC (ctrl->image), 0.5, 0.5);
      if (or == GTK_ORIENTATION_VERTICAL) {
        gtk_table_attach (GTK_TABLE (table), ctrl->image,
			  tab_pos, tab_pos + 1, 0, 1,
			  GTK_EXPAND, 0, 0, 0);
      } else {
        gtk_table_attach (GTK_TABLE (table), ctrl->image,
			  0, 1, tab_pos, tab_pos + 1,
			  0, GTK_EXPAND, 0, 0);
      }
      gtk_widget_show (ctrl->image);
    }
  }

  /* text label */
  if (or == GTK_ORIENTATION_HORIZONTAL)
    str = g_strdup_printf (_("%s:"), track->label);
  else
    str = track->label;
  ctrl->label = gtk_label_new (str);
  if (or == GTK_ORIENTATION_HORIZONTAL) {
    g_free (str);
    gtk_misc_set_alignment (GTK_MISC (ctrl->label), 0.0, 0.5);
  }
  if (or == GTK_ORIENTATION_VERTICAL) {
    gtk_table_attach (table, ctrl->label,
		      tab_pos, tab_pos + 1, 1, 2,
		      GTK_EXPAND, 0, 0, 0);
  } else {
    gtk_table_attach (table, ctrl->label,
		      1, 2, tab_pos, tab_pos + 1,
		      GTK_FILL, GTK_EXPAND, 0, 0);
  }
  gtk_widget_show (ctrl->label);

  return ctrl;
}

static void
gnome_volume_control_track_put_switch (GtkTable *table,
				       gint      tab_pos,
				       GnomeVolumeControlTrack *ctrl,
				       GnomeAppBar *appbar)
{
  GtkWidget *button;
  AtkObject *accessible;
  gchar *accessible_name, *msg;

  /* container box */
  ctrl->buttonbox = gtk_hbox_new (FALSE, 0);
  gtk_table_attach (GTK_TABLE (table), ctrl->buttonbox,
		    tab_pos, tab_pos + 1,
		    4, 5, GTK_EXPAND, 0, 0, 0);
  gtk_widget_show (ctrl->buttonbox);

  /* mute button */
  msg = g_strdup_printf (_("Mute/unmute %s"), ctrl->track->label);
  button = gnome_volume_control_button_new (GNOME_VOLUME_CONTROL_STOCK_PLAY,
					    GNOME_VOLUME_CONTROL_STOCK_NOPLAY,
					    appbar, msg);
  ctrl->mute = GNOME_VOLUME_CONTROL_BUTTON (button);
  g_free (msg);
  gnome_volume_control_button_set_active (GNOME_VOLUME_CONTROL_BUTTON (button),
					  !GST_MIXER_TRACK_HAS_FLAG (ctrl->track,
					       GST_MIXER_TRACK_MUTE));
  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (cb_mute_toggled), ctrl);

  /* a11y */
  accessible = gtk_widget_get_accessible (button);
  if (GTK_IS_ACCESSIBLE (accessible)) {
    accessible_name = g_strdup_printf (_("Track %s: mute"),
				       ctrl->track->label);
    atk_object_set_name (accessible, accessible_name);
    g_free (accessible_name);
  }

  /* show */
  gtk_box_pack_start (GTK_BOX (ctrl->buttonbox), button,
		      FALSE, FALSE, 0);
  gtk_widget_show (button);
}

GnomeVolumeControlTrack *
gnome_volume_control_track_add_playback	(GtkTable *table,
					 gint      tab_pos,
					 GstMixer *mixer,
					 GstMixerTrack *track,
					 GtkWidget *l_sep,
					 GtkWidget *r_sep,
					 GnomeAppBar *appbar)
{
  GnomeVolumeControlTrack *ctrl;
  GtkWidget *slider;
  gint i, *volumes;
  GtkObject *adj;
  AtkObject *accessible;
  gchar *accessible_name;

  /* image, title */
  ctrl = gnome_volume_control_track_add_title (table, tab_pos,
					       GTK_ORIENTATION_VERTICAL,
					       mixer, track, l_sep, r_sep);

  /* switch exception (no sliders) */
  if (track->num_channels == 0) {
    gnome_volume_control_track_put_switch (table, tab_pos, ctrl, appbar);
    return ctrl;
  }

  ctrl->sliderbox = gnome_volume_control_volume_new (ctrl->mixer,
						     ctrl->track, 6,
						     appbar);
  gtk_table_attach (GTK_TABLE (table), ctrl->sliderbox,
		    tab_pos, tab_pos + 1, 2, 3,
		    GTK_EXPAND, GTK_EXPAND | GTK_FILL, 0, 0);
  gtk_widget_show (ctrl->sliderbox);

  /* mute button */
  gnome_volume_control_track_put_switch (table, tab_pos, ctrl, appbar);

  return ctrl;
}

GnomeVolumeControlTrack *
gnome_volume_control_track_add_recording (GtkTable *table,
					  gint      tab_pos,
					  GstMixer *mixer,
					  GstMixerTrack *track,
					  GtkWidget *l_sep,
					  GtkWidget *r_sep,
					  GnomeAppBar *appbar)
{
  GnomeVolumeControlTrack *ctrl;
  GtkWidget *button;
  AtkObject *accessible;
  gchar *accessible_name, *msg;

  ctrl = gnome_volume_control_track_add_playback (table, tab_pos, mixer,
						  track, l_sep, r_sep,
						  appbar);
  if (track->num_channels == 0) {
    return ctrl;
  }

  /* FIXME:
   * - there's something fishy about this button, it
   *     is always FALSE.
   */

  /* only the record button here */
  msg = g_strdup_printf (_("Toggle audio recording from %s"), ctrl->track->label);
  button = gnome_volume_control_button_new (GNOME_VOLUME_CONTROL_STOCK_RECORD,
					    GNOME_VOLUME_CONTROL_STOCK_NORECORD,
					    appbar, msg);
  ctrl->record = GNOME_VOLUME_CONTROL_BUTTON (button);
  g_free (msg);
  gnome_volume_control_button_set_active (GNOME_VOLUME_CONTROL_BUTTON (button),
					  GST_MIXER_TRACK_HAS_FLAG (track,
					      GST_MIXER_TRACK_RECORD));
  g_signal_connect (G_OBJECT (button), "clicked",
		    G_CALLBACK (cb_record_toggled), ctrl);

  /* a11y */
  accessible = gtk_widget_get_accessible (button);
  if (GTK_IS_ACCESSIBLE (accessible)) {
    accessible_name = g_strdup_printf (_("Track %s: audio recording"),
				       track->label);
    atk_object_set_name (accessible, accessible_name);
    g_free (accessible_name);
  }

  /* attach, show */
  gtk_box_pack_start (GTK_BOX (ctrl->buttonbox), button,
		      FALSE, FALSE, 0);
  gtk_widget_show (button);

  return ctrl;
}

GnomeVolumeControlTrack *
gnome_volume_control_track_add_switch (GtkTable *table,
				       gint      tab_pos,
				       GstMixer *mixer,
				       GstMixerTrack *track,
				       GtkWidget *l_sep,
				       GtkWidget *r_sep,
				       GnomeAppBar *appbar)
{
  GnomeVolumeControlTrack *ctrl;

  /* image, title */
  ctrl = gnome_volume_control_track_add_title (table, tab_pos,
					       GTK_ORIENTATION_HORIZONTAL,
					       mixer, track, l_sep, r_sep);
  ctrl->toggle = gtk_check_button_new ();
  if (should_toggle_record_switch (ctrl->track)) {
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ctrl->toggle),
      GST_MIXER_TRACK_HAS_FLAG (ctrl->track, GST_MIXER_TRACK_RECORD));
  } else {
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ctrl->toggle),
      !GST_MIXER_TRACK_HAS_FLAG (ctrl->track, GST_MIXER_TRACK_MUTE));
  }

  /* attach'n'show */
  gtk_table_attach (GTK_TABLE (table), ctrl->toggle,
                    2, 3, tab_pos, tab_pos + 1,
                    GTK_EXPAND | GTK_FILL, GTK_EXPAND, 0, 0);
  g_signal_connect (ctrl->toggle, "toggled",
		    G_CALLBACK (cb_toggle_changed), ctrl);
  gtk_widget_show (ctrl->toggle);
                                                                                
  return ctrl;  
}

GnomeVolumeControlTrack *
gnome_volume_control_track_add_option (GtkTable *table,
				       gint      tab_pos,
				       GstMixer *mixer,
				       GstMixerTrack *track,
				       GtkWidget *l_sep,
				       GtkWidget *r_sep,
				       GnomeAppBar *appbar)
{
  GnomeVolumeControlTrack *ctrl;
  GstMixerOptions *options = GST_MIXER_OPTIONS (track);
  const GList *opt;
  AtkObject *accessible;
  gchar *accessible_name;
  gint i = 0;
  const gchar *active_opt;

  ctrl = gnome_volume_control_track_add_title (table, tab_pos,
					       GTK_ORIENTATION_HORIZONTAL,
					       mixer, track, l_sep, r_sep);

  /* optionmenu */
  active_opt = gst_mixer_get_option (mixer, GST_MIXER_OPTIONS (track));
  ctrl->options = gtk_combo_box_new_text ();
  for (opt = options->values; opt != NULL; opt = opt->next, i++) {
    gtk_combo_box_append_text (GTK_COMBO_BOX (ctrl->options), opt->data);
    if (!strcmp (active_opt, opt->data)) {
      gtk_combo_box_set_active (GTK_COMBO_BOX (ctrl->options), i);
    }
  }

  /* a11y */
  accessible = gtk_widget_get_accessible (ctrl->options);
  if (GTK_IS_ACCESSIBLE (accessible)) {
    accessible_name = g_strdup_printf (_("%s Option Selection"),
				       ctrl->track->label);
    atk_object_set_name (accessible, accessible_name);
    g_free (accessible_name);
  }
  gtk_widget_show (ctrl->options);
  g_signal_connect (ctrl->options, "changed",
		    G_CALLBACK (cb_option_changed), ctrl);

  /* attach'n'show */
  gtk_table_attach (GTK_TABLE (table), ctrl->options,
		    2, 3, tab_pos, tab_pos + 1,
		    GTK_EXPAND | GTK_FILL, GTK_EXPAND, 0, 0);
  gtk_widget_show (ctrl->options);

  return ctrl;
}

void
gnome_volume_control_track_free (GnomeVolumeControlTrack *track)
{
  if (track->id != 0) {
    g_source_remove (track->id);
    track->id = 0;
  }

  g_object_unref (G_OBJECT (track->track));

  g_free (track);
}

void
gnome_volume_control_track_show (GnomeVolumeControlTrack *track,
				 gboolean visible)
{
  if (track->visible == visible)
    return;

#define func(w) \
  if (w != NULL) { \
    if (visible) { \
      gtk_widget_show (w); \
    } else { \
      gtk_widget_hide (w); \
    } \
  }

  func (track->label);
  func (track->image);
  func (track->sliderbox);
  func (track->buttonbox);
  func (track->toggle);
  func (track->options);

  track->visible = visible;

  /* get rid of spacing between hidden tracks */
  if (visible) {
    if (track->options) {
      gtk_table_set_row_spacing (track->table,
				 track->pos, 6);
      if (track->pos > 0)
        gtk_table_set_row_spacing (track->table,
				   track->pos - 1, 6);
    } else if (!track->toggle) {
      gtk_table_set_col_spacing (track->table,
				 track->pos, 6);
      if (track->pos > 0)
        gtk_table_set_col_spacing (track->table,
				   track->pos - 1, 6);
    }
  } else {
    if (track->options) {
      gtk_table_set_row_spacing (track->table,
				 track->pos, 0);
      if (track->pos > 0)
        gtk_table_set_row_spacing (track->table,
				   track->pos - 1, 0);
    } else if (!track->toggle) {
      gtk_table_set_col_spacing (track->table,
				 track->pos, 0);
      if (track->pos > 0)
        gtk_table_set_col_spacing (track->table,
				   track->pos - 1, 0);
    }
  }
}

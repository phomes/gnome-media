/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 William Jon McCann
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include <gconf/gconf-client.h>

#include "gvc-channel-bar.h"
#include "gvc-mixer-control.h"
#include "gvc-mixer-sink.h"
#include "gvc-mixer-source.h"
#include "gvc-mixer-dialog.h"
#include "gvc-sound-theme-chooser.h"

#define SCALE_SIZE 128

#define GVC_MIXER_DIALOG_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GVC_TYPE_MIXER_DIALOG, GvcMixerDialogPrivate))

struct GvcMixerDialogPrivate
{
        GvcMixerControl *mixer_control;
        GHashTable      *bars;
        GtkWidget       *output_bar;
        GtkWidget       *input_bar;
        GtkWidget       *effects_bar;
        GtkWidget       *output_stream_box;
        GtkWidget       *sound_effects_box;
        GtkWidget       *input_box;
        GtkWidget       *output_box;
        GtkWidget       *applications_box;
        GtkWidget       *output_treeview;
        GtkWidget       *input_treeview;
        GtkWidget       *sound_theme_chooser;
        GtkWidget       *enable_effects_button;
        GtkWidget       *click_feedback_button;
        GtkWidget       *audible_bell_button;
        GtkSizeGroup    *size_group;
};

#define KEY_SOUNDS_DIR             "/desktop/gnome/sound"
#define EVENT_SOUNDS_KEY           KEY_SOUNDS_DIR "/event_sounds"
#define INPUT_SOUNDS_KEY           KEY_SOUNDS_DIR "/input_feedback_sounds"
#define KEY_METACITY_DIR           "/apps/metacity/general"
#define AUDIO_BELL_KEY             KEY_METACITY_DIR "/audible_bell"

enum {
        NAME_COL,
        DEVICE_COL,
        ID_COL,
        NUM_COLS
};

enum
{
        PROP_0,
        PROP_MIXER_CONTROL
};

static void     gvc_mixer_dialog_class_init (GvcMixerDialogClass *klass);
static void     gvc_mixer_dialog_init       (GvcMixerDialog      *mixer_dialog);
static void     gvc_mixer_dialog_finalize   (GObject            *object);

G_DEFINE_TYPE (GvcMixerDialog, gvc_mixer_dialog, GTK_TYPE_DIALOG)
static void

gvc_mixer_dialog_set_mixer_control (GvcMixerDialog  *dialog,
                                    GvcMixerControl *control)
{
        g_return_if_fail (GVC_MIXER_DIALOG (dialog));
        g_return_if_fail (GVC_IS_MIXER_CONTROL (control));

        g_object_ref (control);

        if (dialog->priv->mixer_control != NULL) {
                g_object_unref (dialog->priv->mixer_control);
        }

        dialog->priv->mixer_control = control;

        g_object_notify (G_OBJECT (dialog), "mixer-control");
}

static GvcMixerControl *
gvc_mixer_dialog_get_mixer_control (GvcMixerDialog *dialog)
{
        g_return_val_if_fail (GVC_IS_MIXER_DIALOG (dialog), NULL);

        return dialog->priv->mixer_control;
}

static void
gvc_mixer_dialog_set_property (GObject       *object,
                               guint          prop_id,
                               const GValue  *value,
                               GParamSpec    *pspec)
{
        GvcMixerDialog *self = GVC_MIXER_DIALOG (object);

        switch (prop_id) {
        case PROP_MIXER_CONTROL:
                gvc_mixer_dialog_set_mixer_control (self, g_value_get_object (value));
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
gvc_mixer_dialog_get_property (GObject     *object,
                               guint        prop_id,
                               GValue      *value,
                               GParamSpec  *pspec)
{
        GvcMixerDialog *self = GVC_MIXER_DIALOG (object);

        switch (prop_id) {
        case PROP_MIXER_CONTROL:
                g_value_set_object (value, gvc_mixer_dialog_get_mixer_control (self));
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
on_adjustment_value_changed (GtkAdjustment  *adjustment,
                             GvcMixerDialog *dialog)
{
        gdouble         volume;
        GvcMixerStream *stream;

        stream = g_object_get_data (G_OBJECT (adjustment), "gvc-mixer-dialog-stream"),
        volume = gtk_adjustment_get_value (adjustment);
        if (stream != NULL) {
                gvc_mixer_stream_change_volume (stream, (guint)volume);
        }
}

static void
on_bar_is_muted_notify (GObject        *object,
                        GParamSpec     *pspec,
                        GvcMixerDialog *dialog)
{
        gboolean        is_muted;
        GvcMixerStream *stream;

        is_muted = gvc_channel_bar_get_is_muted (GVC_CHANNEL_BAR (object));

        stream = g_object_get_data (object, "gvc-mixer-dialog-stream");
        if (stream != NULL) {
                gvc_mixer_stream_change_is_muted (stream, is_muted);
        } else {
                g_warning ("Unable to find stream for bar");
        }
}

static GtkWidget *
lookup_bar_for_stream (GvcMixerDialog *dialog,
                       GvcMixerStream *stream)
{
        GtkWidget *bar;

        bar = g_hash_table_lookup (dialog->priv->bars, GUINT_TO_POINTER (gvc_mixer_stream_get_id (stream)));

        return bar;
}

static void
on_stream_volume_notify (GObject        *object,
                         GParamSpec     *pspec,
                         GvcMixerDialog *dialog)
{
        GvcMixerStream *stream;
        GtkWidget      *bar;
        GtkAdjustment  *adj;

        stream = GVC_MIXER_STREAM (object);

        bar = lookup_bar_for_stream (dialog, stream);
        adj = GTK_ADJUSTMENT (gvc_channel_bar_get_adjustment (GVC_CHANNEL_BAR (bar)));

        g_signal_handlers_block_by_func (adj,
                                         on_adjustment_value_changed,
                                         dialog);

        gtk_adjustment_set_value (adj,
                                  gvc_mixer_stream_get_volume (stream));

        g_signal_handlers_unblock_by_func (adj,
                                           on_adjustment_value_changed,
                                           dialog);
}

static void
on_stream_is_muted_notify (GObject        *object,
                           GParamSpec     *pspec,
                           GvcMixerDialog *dialog)
{
        GvcMixerStream *stream;
        GtkWidget      *bar;
        gboolean        is_muted;

        stream = GVC_MIXER_STREAM (object);
        bar = lookup_bar_for_stream (dialog, stream);
        is_muted = gvc_mixer_stream_get_is_muted (stream);
        gvc_channel_bar_set_is_muted (GVC_CHANNEL_BAR (bar),
                                      is_muted);

        if (stream == gvc_mixer_control_get_default_sink (dialog->priv->mixer_control)) {
                gtk_widget_set_sensitive (dialog->priv->applications_box,
                                          !is_muted);
        }

}

static void
save_bar_for_stream (GvcMixerDialog *dialog,
                     GvcMixerStream *stream,
                     GtkWidget      *bar)
{
        g_hash_table_insert (dialog->priv->bars,
                             GUINT_TO_POINTER (gvc_mixer_stream_get_id (stream)),
                             bar);
}

static GtkWidget *
create_bar (GvcMixerDialog *dialog)
{
        GtkWidget     *bar;

        bar = gvc_channel_bar_new ();
        gvc_channel_bar_set_size_group (GVC_CHANNEL_BAR (bar),
                                        dialog->priv->size_group);
        gvc_channel_bar_set_orientation (GVC_CHANNEL_BAR (bar),
                                         GTK_ORIENTATION_HORIZONTAL);
        gvc_channel_bar_set_show_mute (GVC_CHANNEL_BAR (bar),
                                       TRUE);
        g_signal_connect (bar,
                          "notify::is-muted",
                          G_CALLBACK (on_bar_is_muted_notify),
                          dialog);
        return bar;
}

static void
bar_set_stream (GvcMixerDialog *dialog,
                GtkWidget      *bar,
                GvcMixerStream *stream)
{
        GtkAdjustment *adj;
        gboolean       is_muted;

        g_assert (stream != NULL);

        gtk_widget_set_sensitive (bar, TRUE);

        is_muted = gvc_mixer_stream_get_is_muted (stream);
        gvc_channel_bar_set_is_muted (GVC_CHANNEL_BAR (bar), is_muted);

        save_bar_for_stream (dialog, stream, bar);

        adj = GTK_ADJUSTMENT (gvc_channel_bar_get_adjustment (GVC_CHANNEL_BAR (bar)));

        gtk_adjustment_set_value (adj,
                                  gvc_mixer_stream_get_volume (stream));

        g_object_set_data (G_OBJECT (bar), "gvc-mixer-dialog-stream", stream);
        g_object_set_data (G_OBJECT (adj), "gvc-mixer-dialog-stream", stream);
        g_signal_connect (adj,
                          "value-changed",
                          G_CALLBACK (on_adjustment_value_changed),
                          dialog);
        gtk_widget_show (bar);
}

static void
add_stream (GvcMixerDialog *dialog,
            GvcMixerStream *stream)
{
        GtkWidget     *bar;
        gboolean       is_muted;

        g_assert (stream != NULL);

        bar = NULL;

        if (stream == gvc_mixer_control_get_default_sink (dialog->priv->mixer_control)) {
                bar = dialog->priv->output_bar;
                is_muted = gvc_mixer_stream_get_is_muted (stream);
                gtk_widget_set_sensitive (dialog->priv->applications_box,
                                          !is_muted);
        } else if (stream == gvc_mixer_control_get_default_source (dialog->priv->mixer_control)) {
                bar = dialog->priv->input_bar;
        } else if (stream == gvc_mixer_control_get_event_sink_input (dialog->priv->mixer_control)) {
                bar = dialog->priv->effects_bar;
        } else if (! GVC_IS_MIXER_SOURCE (stream)
                   && !GVC_IS_MIXER_SINK (stream)) {
                bar = create_bar (dialog);
                gvc_channel_bar_set_name (GVC_CHANNEL_BAR (bar),
                                          gvc_mixer_stream_get_name (stream));
                gvc_channel_bar_set_icon_name (GVC_CHANNEL_BAR (bar),
                                               gvc_mixer_stream_get_icon_name (stream));
                gtk_box_pack_start (GTK_BOX (dialog->priv->applications_box), bar, FALSE, FALSE, 12);
        }

        if (GVC_IS_MIXER_SOURCE (stream)) {
                GtkTreeModel *model;
                GtkTreeIter   iter;
                model = gtk_tree_view_get_model (GTK_TREE_VIEW (dialog->priv->input_treeview));
                gtk_list_store_append (GTK_LIST_STORE (model), &iter);
                gtk_list_store_set (GTK_LIST_STORE (model),
                                    &iter,
                                    NAME_COL, gvc_mixer_stream_get_description (stream),
                                    DEVICE_COL, "",
                                    ID_COL, gvc_mixer_stream_get_id (stream),
                                    -1);
        } else if (GVC_IS_MIXER_SINK (stream)) {
                GtkTreeModel *model;
                GtkTreeIter   iter;
                model = gtk_tree_view_get_model (GTK_TREE_VIEW (dialog->priv->output_treeview));
                gtk_list_store_append (GTK_LIST_STORE (model), &iter);
                gtk_list_store_set (GTK_LIST_STORE (model),
                                    &iter,
                                    NAME_COL, gvc_mixer_stream_get_description (stream),
                                    DEVICE_COL, "",
                                    ID_COL, gvc_mixer_stream_get_id (stream),
                                    -1);
        }

        if (bar != NULL) {
                bar_set_stream (dialog, bar, stream);
        }

        g_signal_connect (stream,
                          "notify::is-muted",
                          G_CALLBACK (on_stream_is_muted_notify),
                          dialog);
        g_signal_connect (stream,
                          "notify::volume",
                          G_CALLBACK (on_stream_volume_notify),
                          dialog);
}

static void
on_control_stream_added (GvcMixerControl *control,
                         guint            id,
                         GvcMixerDialog  *dialog)
{
        GvcMixerStream *stream;
        GtkWidget      *bar;

        bar = g_hash_table_lookup (dialog->priv->bars, GUINT_TO_POINTER (id));
        if (bar != NULL) {
                g_debug ("GvcMixerDialog: Stream %u already added", id);
                return;
        }

        stream = gvc_mixer_control_lookup_stream_id (control, id);
        if (stream != NULL) {
                add_stream (dialog, stream);
        }
}

static gboolean
find_stream_by_id (GtkTreeModel *model,
                   guint         id,
                   GtkTreeIter  *iter)
{
        gboolean found_item;

        found_item = FALSE;

        if (!gtk_tree_model_get_iter_first (model, iter)) {
                return FALSE;
        }

        do {
                guint t_id;

                gtk_tree_model_get (model, iter,
                                    ID_COL, &t_id, -1);

                if (id == t_id) {
                        found_item = TRUE;
                }
        } while (!found_item && gtk_tree_model_iter_next (model, iter));

        return found_item;
}

static void
on_control_stream_removed (GvcMixerControl *control,
                           guint            id,
                           GvcMixerDialog  *dialog)
{
        GtkWidget    *bar;
        gboolean      found;
        GtkTreeIter   iter;
        GtkTreeModel *model;

        bar = g_hash_table_lookup (dialog->priv->bars, GUINT_TO_POINTER (id));
        if (bar != NULL) {
                g_hash_table_remove (dialog->priv->bars, GUINT_TO_POINTER (id));
                gtk_container_remove (GTK_CONTAINER (bar->parent),
                                      bar);
        }

        model = gtk_tree_view_get_model (GTK_TREE_VIEW (dialog->priv->output_treeview));
        found = find_stream_by_id (GTK_TREE_MODEL (model), id, &iter);
        if (found) {
                gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
        }
        model = gtk_tree_view_get_model (GTK_TREE_VIEW (dialog->priv->input_treeview));
        found = find_stream_by_id (GTK_TREE_MODEL (model), id, &iter);
        if (found) {
                gtk_list_store_remove (GTK_LIST_STORE (model), &iter);
        }
}

static void
on_enable_effects_toggled (GtkToggleButton *button,
                           GvcMixerDialog  *dialog)
{
        GConfClient *client;
        gboolean     enabled;

        enabled = gtk_toggle_button_get_active (button);

        client = gconf_client_get_default ();
        gconf_client_set_bool (client, EVENT_SOUNDS_KEY, enabled, NULL);
        g_object_unref (client);
}

static void
on_click_feedback_toggled (GtkToggleButton *button,
                           GvcMixerDialog  *dialog)
{
        GConfClient *client;
        gboolean     enabled;

        enabled = gtk_toggle_button_get_active (button);

        client = gconf_client_get_default ();
        gconf_client_set_bool (client, INPUT_SOUNDS_KEY, enabled, NULL);
        g_object_unref (client);
}

static void
on_audible_bell_toggled (GtkToggleButton *button,
                         GvcMixerDialog  *dialog)
{
        GConfClient *client;
        gboolean     enabled;

        enabled = gtk_toggle_button_get_active (button);

        client = gconf_client_get_default ();
        gconf_client_set_bool (client, AUDIO_BELL_KEY, enabled, NULL);
        g_object_unref (client);
}

static void
_gtk_label_make_bold (GtkLabel *label)
{
        PangoFontDescription *font_desc;

        font_desc = pango_font_description_new ();

        pango_font_description_set_weight (font_desc,
                                           PANGO_WEIGHT_BOLD);

        /* This will only affect the weight of the font, the rest is
         * from the current state of the widget, which comes from the
         * theme or user prefs, since the font desc only has the
         * weight flag turned on.
         */
        gtk_widget_modify_font (GTK_WIDGET (label), font_desc);

        pango_font_description_free (font_desc);
}

static GtkWidget *
create_stream_treeview (GvcMixerDialog *dialog)
{
        GtkWidget         *treeview;
        GtkListStore      *store;
        GtkCellRenderer   *renderer;
        GtkTreeViewColumn *column;

        treeview = gtk_tree_view_new ();
        gtk_tree_view_set_headers_visible (GTK_TREE_VIEW (treeview), TRUE);

        store = gtk_list_store_new (NUM_COLS,
                                    G_TYPE_STRING,
                                    G_TYPE_STRING,
                                    G_TYPE_UINT);
        gtk_tree_view_set_model (GTK_TREE_VIEW (treeview),
                                 GTK_TREE_MODEL (store));

        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Name"),
                                                           renderer,
                                                           "text", NAME_COL,
                                                           NULL);
        gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Device"),
                                                           renderer,
                                                           "text", DEVICE_COL,
                                                           NULL);
        gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

        return treeview;
}

static GObject *
gvc_mixer_dialog_constructor (GType                  type,
                              guint                  n_construct_properties,
                              GObjectConstructParam *construct_params)
{
        GObject        *object;
        GvcMixerDialog *self;
        GtkWidget      *main_vbox;
        GtkWidget      *label;
        GtkWidget      *alignment;
        GtkWidget      *box;
        GtkWidget      *notebook;
        GSList         *streams;
        GSList         *l;
        GvcMixerStream *stream;
        GConfClient    *client;

        object = G_OBJECT_CLASS (gvc_mixer_dialog_parent_class)->constructor (type, n_construct_properties, construct_params);

        self = GVC_MIXER_DIALOG (object);

#if GTK_CHECK_VERSION(2,14,0)
        main_vbox = gtk_dialog_get_content_area (GTK_DIALOG (self));
#else
        main_vbox = GTK_DIALOG (self)->vbox;
#endif

        gtk_container_set_border_width (GTK_CONTAINER (self), 12);

        self->priv->output_stream_box = gtk_hbox_new (FALSE, 12);
        gtk_box_pack_start (GTK_BOX (main_vbox),
                            self->priv->output_stream_box,
                            FALSE, FALSE, 12);
        self->priv->output_bar = create_bar (self);
        gvc_channel_bar_set_name (GVC_CHANNEL_BAR (self->priv->output_bar),
                                  _("Output volume: "));
        gtk_widget_set_sensitive (self->priv->output_bar, FALSE);
        gtk_box_pack_start (GTK_BOX (self->priv->output_stream_box),
                            self->priv->output_bar, TRUE, TRUE, 12);

        notebook = gtk_notebook_new ();
        gtk_box_pack_start (GTK_BOX (main_vbox),
                            notebook,
                            TRUE, TRUE, 12);

        /* Effects page */
        self->priv->sound_effects_box = gtk_vbox_new (FALSE, 12);
        gtk_container_set_border_width (GTK_CONTAINER (self->priv->sound_effects_box), 12);
        label = gtk_label_new (_("Sound Effects"));
        gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
                                  self->priv->sound_effects_box,
                                  label);

        self->priv->effects_bar = create_bar (self);
        gvc_channel_bar_set_name (GVC_CHANNEL_BAR (self->priv->effects_bar),
                                  _("Alert Volume: "));
        gtk_widget_set_sensitive (self->priv->effects_bar, FALSE);
        gtk_box_pack_end (GTK_BOX (self->priv->sound_effects_box),
                          self->priv->effects_bar, FALSE, FALSE, 12);

        self->priv->sound_theme_chooser = gvc_sound_theme_chooser_new ();
        gtk_box_pack_start (GTK_BOX (self->priv->sound_effects_box),
                            self->priv->sound_theme_chooser,
                            TRUE, TRUE, 0);

        client = gconf_client_get_default ();

        self->priv->enable_effects_button = gtk_check_button_new_with_mnemonic (_("_Play alerts and sound effects"));
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->priv->enable_effects_button),
                                      gconf_client_get_bool (client, EVENT_SOUNDS_KEY, NULL));
        gtk_box_pack_start (GTK_BOX (self->priv->sound_effects_box),
                            self->priv->enable_effects_button,
                            FALSE, FALSE, 0);
        g_signal_connect (self->priv->enable_effects_button,
                          "toggled",
                          G_CALLBACK (on_enable_effects_toggled),
                          self);
        alignment = gtk_alignment_new (0, 0, 1, 1);
        gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 0, 0, 12, 0);
        gtk_box_pack_start (GTK_BOX (self->priv->sound_effects_box),
                            alignment,
                            FALSE, FALSE, 0);
        box = gtk_vbox_new (FALSE, 6);
        gtk_container_add (GTK_CONTAINER (alignment), box);
        self->priv->click_feedback_button = gtk_check_button_new_with_mnemonic (_("Play _sound effects when buttons are clicked"));
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->priv->click_feedback_button),
                                      gconf_client_get_bool (client, INPUT_SOUNDS_KEY, NULL));
        gtk_box_pack_start (GTK_BOX (box),
                            self->priv->click_feedback_button,
                            FALSE, FALSE, 0);
        g_signal_connect (self->priv->click_feedback_button,
                          "toggled",
                          G_CALLBACK (on_click_feedback_toggled),
                          self);
        self->priv->audible_bell_button = gtk_check_button_new_with_mnemonic (_("Play _alert sound"));
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (self->priv->audible_bell_button),
                                      gconf_client_get_bool (client, AUDIO_BELL_KEY, NULL));
        gtk_box_pack_start (GTK_BOX (box),
                            self->priv->audible_bell_button,
                            FALSE, FALSE, 0);
        g_signal_connect (self->priv->audible_bell_button,
                          "toggled",
                          G_CALLBACK (on_audible_bell_toggled),
                          self);

        /* Output page */
        self->priv->output_box = gtk_vbox_new (FALSE, 12);
        gtk_container_set_border_width (GTK_CONTAINER (self->priv->output_box), 12);
        label = gtk_label_new (_("Output"));
        gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
                                  self->priv->output_box,
                                  label);

        box = gtk_frame_new (_("Choose a device for sound output"));
        label = gtk_frame_get_label_widget (GTK_FRAME (box));
        _gtk_label_make_bold (GTK_LABEL (label));
        gtk_frame_set_shadow_type (GTK_FRAME (box), GTK_SHADOW_NONE);
        gtk_box_pack_start (GTK_BOX (self->priv->output_box), box, TRUE, TRUE, 0);

        alignment = gtk_alignment_new (0, 0, 1, 1);
        gtk_container_add (GTK_CONTAINER (box), alignment);
        gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 6, 0, 0, 0);

        self->priv->output_treeview = create_stream_treeview (self);
        box = gtk_scrolled_window_new (NULL, NULL);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (box),
                                        GTK_POLICY_NEVER,
                                        GTK_POLICY_AUTOMATIC);
        gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (box),
                                             GTK_SHADOW_IN);
        gtk_container_add (GTK_CONTAINER (box), self->priv->output_treeview);
        gtk_container_add (GTK_CONTAINER (alignment), box);


        /* Input page */
        self->priv->input_box = gtk_vbox_new (FALSE, 12);
        gtk_container_set_border_width (GTK_CONTAINER (self->priv->input_box), 12);
        label = gtk_label_new (_("Input"));
        gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
                                  self->priv->input_box,
                                  label);

        self->priv->input_bar = create_bar (self);
        gvc_channel_bar_set_name (GVC_CHANNEL_BAR (self->priv->input_bar),
                                  _("Input volume: "));
        gtk_widget_set_sensitive (self->priv->input_bar, FALSE);
        gtk_box_pack_end (GTK_BOX (self->priv->input_box),
                          self->priv->input_bar, FALSE, FALSE, 12);

        box = gtk_frame_new (_("Choose a device for sound input"));
        label = gtk_frame_get_label_widget (GTK_FRAME (box));
        _gtk_label_make_bold (GTK_LABEL (label));
        gtk_frame_set_shadow_type (GTK_FRAME (box), GTK_SHADOW_NONE);
        gtk_box_pack_start (GTK_BOX (self->priv->input_box), box, TRUE, TRUE, 0);

        alignment = gtk_alignment_new (0, 0, 1, 1);
        gtk_container_add (GTK_CONTAINER (box), alignment);
        gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 6, 0, 0, 0);

        self->priv->input_treeview = create_stream_treeview (self);
        box = gtk_scrolled_window_new (NULL, NULL);
        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (box),
                                        GTK_POLICY_NEVER,
                                        GTK_POLICY_AUTOMATIC);
        gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (box),
                                             GTK_SHADOW_IN);
        gtk_container_add (GTK_CONTAINER (box), self->priv->input_treeview);
        gtk_container_add (GTK_CONTAINER (alignment), box);


        /* Applications */
        self->priv->applications_box = gtk_vbox_new (FALSE, 12);
        gtk_container_set_border_width (GTK_CONTAINER (self->priv->applications_box), 12);
        label = gtk_label_new (_("Applications"));
        gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
                                  self->priv->applications_box,
                                  label);

        gtk_widget_show_all (GTK_WIDGET (self));

        g_signal_connect (self->priv->mixer_control,
                          "stream-added",
                          G_CALLBACK (on_control_stream_added),
                          self);
        g_signal_connect (self->priv->mixer_control,
                          "stream-removed",
                          G_CALLBACK (on_control_stream_removed),
                          self);

        streams = gvc_mixer_control_get_streams (self->priv->mixer_control);
        for (l = streams; l != NULL; l = l->next) {
                stream = l->data;
                add_stream (self, stream);
        }
        g_slist_free (streams);

        return object;
}

static void
gvc_mixer_dialog_dispose (GObject *object)
{
        GvcMixerDialog *dialog = GVC_MIXER_DIALOG (object);

        g_signal_handlers_disconnect_by_func (dialog->priv->mixer_control,
                                              on_control_stream_added,
                                              dialog);
        g_signal_handlers_disconnect_by_func (dialog->priv->mixer_control,
                                              on_control_stream_removed,
                                              dialog);

        if (dialog->priv->mixer_control != NULL) {
                g_object_unref (dialog->priv->mixer_control);
                dialog->priv->mixer_control = NULL;
        }

        if (dialog->priv->bars != NULL) {
                g_hash_table_destroy (dialog->priv->bars);
                dialog->priv->bars = NULL;
        }

        G_OBJECT_CLASS (gvc_mixer_dialog_parent_class)->dispose (object);
}

static void
gvc_mixer_dialog_class_init (GvcMixerDialogClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);

        object_class->constructor = gvc_mixer_dialog_constructor;
        object_class->dispose = gvc_mixer_dialog_dispose;
        object_class->finalize = gvc_mixer_dialog_finalize;
        object_class->set_property = gvc_mixer_dialog_set_property;
        object_class->get_property = gvc_mixer_dialog_get_property;

        g_object_class_install_property (object_class,
                                         PROP_MIXER_CONTROL,
                                         g_param_spec_object ("mixer-control",
                                                              "mixer control",
                                                              "mixer control",
                                                              GVC_TYPE_MIXER_CONTROL,
                                                              G_PARAM_READWRITE|G_PARAM_CONSTRUCT));

        g_type_class_add_private (klass, sizeof (GvcMixerDialogPrivate));
}

static void
on_key_changed (GConfClient    *client,
                guint           cnxn_id,
                GConfEntry     *entry,
                GvcMixerDialog *dialog)
{
        const char *key;
        GConfValue *value;
        gboolean    enabled;

        key = gconf_entry_get_key (entry);

        if (! g_str_has_prefix (key, KEY_SOUNDS_DIR)
            && ! g_str_has_prefix (key, KEY_METACITY_DIR)) {
                return;
        }

        value = gconf_entry_get_value (entry);
        enabled = gconf_value_get_bool (value);
        if (strcmp (key, EVENT_SOUNDS_KEY) == 0) {
                gtk_widget_set_sensitive (dialog->priv->sound_theme_chooser, enabled);
                gtk_widget_set_sensitive (dialog->priv->click_feedback_button, enabled);
                gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->priv->enable_effects_button), enabled);
        } else if (strcmp (key, INPUT_SOUNDS_KEY) == 0) {
                gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->priv->click_feedback_button), enabled);
        } else if (strcmp (key, AUDIO_BELL_KEY) == 0) {
                gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (dialog->priv->audible_bell_button), enabled);
        }
}

static void
gvc_mixer_dialog_init (GvcMixerDialog *dialog)
{
        GConfClient *client;

        dialog->priv = GVC_MIXER_DIALOG_GET_PRIVATE (dialog);
        dialog->priv->bars = g_hash_table_new (NULL, NULL);
        dialog->priv->size_group = gtk_size_group_new (GTK_SIZE_GROUP_HORIZONTAL);

        client = gconf_client_get_default ();

        gconf_client_add_dir (client, KEY_SOUNDS_DIR,
                              GCONF_CLIENT_PRELOAD_ONELEVEL,
                              NULL);
        gconf_client_notify_add (client,
                                 KEY_SOUNDS_DIR,
                                 (GConfClientNotifyFunc)on_key_changed,
                                 dialog, NULL, NULL);
        gconf_client_add_dir (client, KEY_METACITY_DIR,
                              GCONF_CLIENT_PRELOAD_ONELEVEL,
                              NULL);
        gconf_client_notify_add (client,
                                 KEY_METACITY_DIR,
                                 (GConfClientNotifyFunc)on_key_changed,
                                 dialog, NULL, NULL);

        g_object_unref (client);
}

static void
gvc_mixer_dialog_finalize (GObject *object)
{
        GvcMixerDialog *mixer_dialog;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GVC_IS_MIXER_DIALOG (object));

        mixer_dialog = GVC_MIXER_DIALOG (object);

        g_return_if_fail (mixer_dialog->priv != NULL);
        G_OBJECT_CLASS (gvc_mixer_dialog_parent_class)->finalize (object);
}

GvcMixerDialog *
gvc_mixer_dialog_new (GvcMixerControl *control)
{
        GObject *dialog;
        dialog = g_object_new (GVC_TYPE_MIXER_DIALOG,
                               "icon-name", "multimedia-volume-control",
                               "title", _("Sound Preferences"),
                               "has-separator", FALSE,
                               "mixer-control", control,
                               NULL);
        return GVC_MIXER_DIALOG (dialog);
}

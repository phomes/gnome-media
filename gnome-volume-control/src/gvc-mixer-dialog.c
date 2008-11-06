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
        GtkWidget       *output_stream_box;
        GtkWidget       *sound_effects_box;
        GtkWidget       *input_box;
        GtkWidget       *output_box;
        GtkWidget       *applications_box;
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

        stream = g_object_get_data (object, "gvc-mixer-dialog-stream");
        is_muted = gvc_channel_bar_get_is_muted (GVC_CHANNEL_BAR (object));
        if (stream != NULL) {
                gvc_mixer_stream_change_is_muted (stream, is_muted);
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

static void
add_stream (GvcMixerDialog *dialog,
            GvcMixerStream *stream)
{
        GtkWidget     *bar;
        GtkAdjustment *adj;
        gboolean       is_muted;

        bar = gvc_channel_bar_new ();
        gvc_channel_bar_set_orientation (GVC_CHANNEL_BAR (bar),
                                         GTK_ORIENTATION_HORIZONTAL);
        gvc_channel_bar_set_show_mute (GVC_CHANNEL_BAR (bar),
                                       TRUE);
        is_muted = gvc_mixer_stream_get_is_muted (stream);

        if (stream == gvc_mixer_control_get_default_sink (dialog->priv->mixer_control)) {
                gvc_channel_bar_set_name (GVC_CHANNEL_BAR (bar),
                                          _("Output volume: "));
                gtk_widget_set_sensitive (dialog->priv->applications_box,
                                          !is_muted);
        } else if (stream == gvc_mixer_control_get_default_source (dialog->priv->mixer_control)) {
                gvc_channel_bar_set_name (GVC_CHANNEL_BAR (bar),
                                          _("Input volume: "));
        } else if (stream == gvc_mixer_control_get_event_sink_input (dialog->priv->mixer_control)) {
                gvc_channel_bar_set_name (GVC_CHANNEL_BAR (bar),
                                          _("Alert volume: "));
        } else {
                gvc_channel_bar_set_name (GVC_CHANNEL_BAR (bar),
                                          gvc_mixer_stream_get_name (stream));
                gvc_channel_bar_set_icon_name (GVC_CHANNEL_BAR (bar),
                                               gvc_mixer_stream_get_icon_name (stream));
        }

        g_object_set_data (G_OBJECT (bar), "gvc-mixer-dialog-stream", stream);

        save_bar_for_stream (dialog, stream, bar);

        if (GVC_IS_MIXER_SINK (stream)) {
                gtk_box_pack_start (GTK_BOX (dialog->priv->output_stream_box), bar, TRUE, TRUE, 12);
        } else if (GVC_IS_MIXER_SOURCE (stream)) {
                gtk_box_pack_end (GTK_BOX (dialog->priv->input_box), bar, FALSE, FALSE, 12);
        } else if (stream == gvc_mixer_control_get_event_sink_input (dialog->priv->mixer_control)) {
                gtk_box_pack_end (GTK_BOX (dialog->priv->sound_effects_box), bar, FALSE, FALSE, 12);
        } else {
                gtk_box_pack_start (GTK_BOX (dialog->priv->applications_box), bar, FALSE, FALSE, 12);
        }

        gvc_channel_bar_set_is_muted (GVC_CHANNEL_BAR (bar), is_muted);

        adj = GTK_ADJUSTMENT (gvc_channel_bar_get_adjustment (GVC_CHANNEL_BAR (bar)));

        gtk_adjustment_set_value (adj,
                                  gvc_mixer_stream_get_volume (stream));

        g_object_set_data (G_OBJECT (adj), "gvc-mixer-dialog-stream", stream);
        g_signal_connect (adj,
                          "value-changed",
                          G_CALLBACK (on_adjustment_value_changed),
                          dialog);

        g_signal_connect (bar,
                          "notify::is-muted",
                          G_CALLBACK (on_bar_is_muted_notify),
                          dialog);
        g_signal_connect (stream,
                          "notify::is-muted",
                          G_CALLBACK (on_stream_is_muted_notify),
                          dialog);
        g_signal_connect (stream,
                          "notify::volume",
                          G_CALLBACK (on_stream_volume_notify),
                          dialog);
        gtk_widget_show (bar);
}

static void
on_control_stream_added (GvcMixerControl *control,
                         guint            id,
                         GvcMixerDialog  *dialog)
{
        g_debug ("GvcMixerDialog: Stream %u added", id);
}

static void
on_control_stream_removed (GvcMixerControl *control,
                           guint            id,
                           GvcMixerDialog  *dialog)
{
        g_debug ("GvcMixerDialog: Stream %u removed", id);
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
        GtkWidget      *button;
        GtkWidget      *alignment;
        GtkWidget      *box;
        GtkWidget      *notebook;
        GSList         *streams;
        GSList         *l;
        GvcMixerStream *stream;

        object = G_OBJECT_CLASS (gvc_mixer_dialog_parent_class)->constructor (type, n_construct_properties, construct_params);

        self = GVC_MIXER_DIALOG (object);

#if GTK_CHECK_VERSION(2,14,0)
        main_vbox = gtk_dialog_get_content_area (GTK_DIALOG (self));
#else
        main_vbox = GTK_DIALOG (self)->vbox;
#endif

        gtk_container_set_border_width (GTK_CONTAINER (self), 5);

        notebook = gtk_notebook_new ();
        gtk_box_pack_start (GTK_BOX (main_vbox),
                            notebook,
                            TRUE, TRUE, 6);

        self->priv->output_stream_box = gtk_hbox_new (FALSE, 6);
        gtk_box_pack_start (GTK_BOX (main_vbox),
                            self->priv->output_stream_box,
                            FALSE, FALSE, 6);

        /* Effects page */
        self->priv->sound_effects_box = gtk_vbox_new (FALSE, 6);
        gtk_container_set_border_width (GTK_CONTAINER (self->priv->sound_effects_box), 12);
        label = gtk_label_new (_("Sound Effects"));
        gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
                                  self->priv->sound_effects_box,
                                  label);
        box = gvc_sound_theme_chooser_new ();
        gtk_box_pack_start (GTK_BOX (self->priv->sound_effects_box),
                            box,
                            TRUE, TRUE, 6);
        button = gtk_check_button_new_with_mnemonic (_("_Play alerts and sound effects"));
        gtk_box_pack_start (GTK_BOX (self->priv->sound_effects_box),
                            button,
                            FALSE, FALSE, 0);
        alignment = gtk_alignment_new (0, 0, 1, 1);
        gtk_alignment_set_padding (GTK_ALIGNMENT (alignment), 0, 0, 12, 0);
        gtk_box_pack_start (GTK_BOX (self->priv->sound_effects_box),
                            alignment,
                            FALSE, FALSE, 0);
        box = gtk_vbox_new (FALSE, 6);
        gtk_container_add (GTK_CONTAINER (alignment), box);
        button = gtk_check_button_new_with_mnemonic (_("Play _sound effects when buttons are clicked"));
        gtk_box_pack_start (GTK_BOX (box),
                            button,
                            FALSE, FALSE, 0);
        button = gtk_check_button_new_with_mnemonic (_("Play _alert sound"));
        gtk_box_pack_start (GTK_BOX (box),
                            button,
                            FALSE, FALSE, 0);

        /* Output page */
        self->priv->output_box = gtk_vbox_new (FALSE, 6);
        gtk_container_set_border_width (GTK_CONTAINER (self->priv->output_box), 12);
        label = gtk_label_new (_("Output"));
        gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
                                  self->priv->output_box,
                                  label);
        self->priv->input_box = gtk_vbox_new (FALSE, 6);
        gtk_container_set_border_width (GTK_CONTAINER (self->priv->input_box), 12);
        label = gtk_label_new (_("Input"));
        gtk_notebook_append_page (GTK_NOTEBOOK (notebook),
                                  self->priv->input_box,
                                  label);
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

        stream = gvc_mixer_control_get_default_sink (self->priv->mixer_control);
        add_stream (self, stream);

        stream = gvc_mixer_control_get_default_source (self->priv->mixer_control);
        add_stream (self, stream);

        stream = gvc_mixer_control_get_event_sink_input (self->priv->mixer_control);
        add_stream (self, stream);

        streams = gvc_mixer_control_get_sink_inputs (self->priv->mixer_control);
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
gvc_mixer_dialog_init (GvcMixerDialog *dialog)
{
        dialog->priv = GVC_MIXER_DIALOG_GET_PRIVATE (dialog);
        dialog->priv->bars = g_hash_table_new (NULL, NULL);
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

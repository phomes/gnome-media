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
#include "gvc-mixer-dialog.h"

#define SCALE_SIZE 128

#define GVC_MIXER_DIALOG_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GVC_TYPE_MIXER_DIALOG, GvcMixerDialogPrivate))

struct GvcMixerDialogPrivate
{
        GvcMixerControl *mixer_control;
        GHashTable      *bars;
        GtkWidget       *streams_box;
        GtkWidget       *output_streams_box;
        GtkWidget       *application_streams_box;
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
                gtk_widget_set_sensitive (dialog->priv->application_streams_box,
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

        is_muted = gvc_mixer_stream_get_is_muted (stream);

        if (stream == gvc_mixer_control_get_default_sink (dialog->priv->mixer_control)) {
                gvc_channel_bar_set_name (GVC_CHANNEL_BAR (bar),
                                          _("Speakers"));
                gtk_widget_set_sensitive (dialog->priv->application_streams_box,
                                          !is_muted);
        } else {
                gvc_channel_bar_set_name (GVC_CHANNEL_BAR (bar),
                                          gvc_mixer_stream_get_name (stream));
        }

        gvc_channel_bar_set_icon_name (GVC_CHANNEL_BAR (bar),
                                       gvc_mixer_stream_get_icon_name (stream));
        g_object_set_data (G_OBJECT (bar), "gvc-mixer-dialog-stream", stream);

        save_bar_for_stream (dialog, stream, bar);

        if (GVC_IS_MIXER_SINK (stream)) {
                gtk_box_pack_start (GTK_BOX (dialog->priv->output_streams_box), bar, TRUE, FALSE, 0);
        } else {
                gtk_box_pack_start (GTK_BOX (dialog->priv->application_streams_box), bar, TRUE, FALSE, 0);
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

static GObject *
gvc_mixer_dialog_constructor (GType                  type,
                              guint                  n_construct_properties,
                              GObjectConstructParam *construct_params)
{
        GObject        *object;
        GvcMixerDialog *self;
        GtkWidget      *separator;
        GtkWidget      *main_vbox;
        GSList         *streams;
        GSList         *l;
        GvcMixerStream *stream;

        object = G_OBJECT_CLASS (gvc_mixer_dialog_parent_class)->constructor (type, n_construct_properties, construct_params);

        self = GVC_MIXER_DIALOG (object);

        main_vbox = gtk_dialog_get_content_area (GTK_DIALOG (self));
        self->priv->streams_box = gtk_hbox_new (FALSE, 12);
        gtk_container_add (GTK_CONTAINER (main_vbox), self->priv->streams_box);

        self->priv->output_streams_box = gtk_hbox_new (FALSE, 12);
        gtk_box_pack_start (GTK_BOX (self->priv->streams_box),
                            self->priv->output_streams_box,
                            FALSE, FALSE, 6);

        separator = gtk_vseparator_new ();
        gtk_box_pack_start (GTK_BOX (self->priv->streams_box),
                            separator,
                            FALSE, FALSE, 6);

        self->priv->application_streams_box = gtk_hbox_new (FALSE, 12);
        gtk_box_pack_start (GTK_BOX (self->priv->streams_box),
                            self->priv->application_streams_box,
                            FALSE, FALSE, 6);

        gtk_widget_show_all (self->priv->streams_box);

        streams = gvc_mixer_control_get_sinks (self->priv->mixer_control);
        for (l = streams; l != NULL; l = l->next) {
                stream = l->data;
                add_stream (self, stream);
        }
        g_slist_free (streams);

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
gvc_mixer_dialog_class_init (GvcMixerDialogClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);

        object_class->constructor = gvc_mixer_dialog_constructor;
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
                               "title", _("Volume Control"),
                               "has-separator", FALSE,
                               "mixer-control", control,
                               NULL);
        return GVC_MIXER_DIALOG (dialog);
}

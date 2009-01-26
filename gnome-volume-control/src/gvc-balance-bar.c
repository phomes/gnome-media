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

#include "gvc-balance-bar.h"

#define SCALE_SIZE 128

#define GVC_BALANCE_BAR_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GVC_TYPE_BALANCE_BAR, GvcBalanceBarPrivate))

struct GvcBalanceBarPrivate
{
        GvcChannelMap *channel_map;
        GtkWidget     *scale_box;
        GtkWidget     *start_box;
        GtkWidget     *end_box;
        GtkWidget     *label;
        GtkWidget     *scale;
        GtkAdjustment *adjustment;
        GtkSizeGroup  *size_group;
        gboolean       symmetric;
};

enum
{
        PROP_0,
        PROP_CHANNEL_MAP
};

static void     gvc_balance_bar_class_init (GvcBalanceBarClass *klass);
static void     gvc_balance_bar_init       (GvcBalanceBar      *balance_bar);
static void     gvc_balance_bar_finalize   (GObject            *object);

G_DEFINE_TYPE (GvcBalanceBar, gvc_balance_bar, GTK_TYPE_HBOX)

static GtkWidget *
_scale_box_new (GvcBalanceBar *bar)
{
        GvcBalanceBarPrivate *priv = bar->priv;
        GtkWidget            *box;
        GtkWidget            *sbox;
        GtkWidget            *ebox;
        GtkAdjustment        *adjustment = bar->priv->adjustment;
        char                 *str;

        bar->priv->scale_box = box = gtk_hbox_new (FALSE, 6);
        priv->scale = gtk_hscale_new (priv->adjustment);
        gtk_widget_set_size_request (priv->scale, SCALE_SIZE, -1);

        gtk_widget_set_name (priv->scale, "balance-bar-scale");
        gtk_rc_parse_string ("style \"balance-bar-scale-style\" {\n"
                             " GtkScale::trough-side-details = 0\n"
                             "}\n"
                             "widget \"*.balance-bar-scale\" style : rc \"balance-bar-scale-style\"\n");

        bar->priv->start_box = sbox = gtk_hbox_new (FALSE, 6);
        gtk_box_pack_start (GTK_BOX (box), sbox, FALSE, FALSE, 0);

        gtk_box_pack_start (GTK_BOX (sbox), priv->label, FALSE, FALSE, 0);

        gtk_box_pack_start (GTK_BOX (sbox), priv->scale, TRUE, TRUE, 0);

        str = g_strdup_printf ("<small>%s</small>", C_("balance", "Left"));
        gtk_scale_add_mark (GTK_SCALE (priv->scale), adjustment->lower , 
                            GTK_POS_BOTTOM, str);
        g_free (str);

        str = g_strdup_printf ("<small>%s</small>", C_("balance", "Right"));
        gtk_scale_add_mark (GTK_SCALE (priv->scale),  adjustment->upper, 
                            GTK_POS_BOTTOM, str);
        g_free (str);

        gtk_scale_add_mark (GTK_SCALE (priv->scale), (adjustment->upper - adjustment->lower)/2 + adjustment->lower, 
                            GTK_POS_BOTTOM, NULL);

        bar->priv->end_box = ebox = gtk_hbox_new (FALSE, 6);
        gtk_box_pack_start (GTK_BOX (box), ebox, FALSE, FALSE, 0);

        gtk_range_set_update_policy (GTK_RANGE (priv->scale), GTK_UPDATE_DISCONTINUOUS);

        if (bar->priv->size_group != NULL) {
                gtk_size_group_add_widget (bar->priv->size_group, sbox);

                if (bar->priv->symmetric) {
                        gtk_size_group_add_widget (bar->priv->size_group, ebox);
                }
        }

        gtk_scale_set_draw_value (GTK_SCALE (priv->scale), FALSE);

        return box;
}

void
gvc_balance_bar_set_size_group (GvcBalanceBar *bar,
                                GtkSizeGroup  *group,
                                gboolean       symmetric)
{
        g_return_if_fail (GVC_IS_BALANCE_BAR (bar));

        bar->priv->size_group = group;
        bar->priv->symmetric = symmetric;

        if (bar->priv->size_group != NULL) {
                gtk_size_group_add_widget (bar->priv->size_group,
                                           bar->priv->start_box);

                if (bar->priv->symmetric) {
                        gtk_size_group_add_widget (bar->priv->size_group,
                                                   bar->priv->end_box);
                }
        }
        gtk_widget_queue_draw (GTK_WIDGET (bar));
}

static void
gvc_balance_bar_set_channel_map (GvcBalanceBar *bar,
                                 GvcChannelMap *map)
{
        g_return_if_fail (GVC_BALANCE_BAR (bar));

        if (bar->priv->channel_map != NULL) {
                g_object_unref (bar->priv->channel_map);
        }
        bar->priv->channel_map = g_object_ref (map);

        g_object_notify (G_OBJECT (bar), "channel-map");
}

static void
gvc_balance_bar_set_property (GObject       *object,
                              guint          prop_id,
                              const GValue  *value,
                              GParamSpec    *pspec)
{
        GvcBalanceBar *self = GVC_BALANCE_BAR (object);

        switch (prop_id) {
        case PROP_CHANNEL_MAP:
                gvc_balance_bar_set_channel_map (self, g_value_get_object (value));
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
gvc_balance_bar_get_property (GObject     *object,
                              guint        prop_id,
                              GValue      *value,
                              GParamSpec  *pspec)
{
        GvcBalanceBar *self = GVC_BALANCE_BAR (object);

        switch (prop_id) {
        case PROP_CHANNEL_MAP:
                g_value_set_object (value, self->priv->channel_map);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static GObject *
gvc_balance_bar_constructor (GType                  type,
                             guint                  n_construct_properties,
                             GObjectConstructParam *construct_params)
{
        GObject       *object;
        GvcBalanceBar *self;

        object = G_OBJECT_CLASS (gvc_balance_bar_parent_class)->constructor (type, n_construct_properties, construct_params);

        self = GVC_BALANCE_BAR (object);

        return object;
}

static void
gvc_balance_bar_class_init (GvcBalanceBarClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);

        object_class->constructor = gvc_balance_bar_constructor;
        object_class->finalize = gvc_balance_bar_finalize;
        object_class->set_property = gvc_balance_bar_set_property;
        object_class->get_property = gvc_balance_bar_get_property;

        g_object_class_install_property (object_class,
                                         PROP_CHANNEL_MAP,
                                         g_param_spec_object ("channel-map",
                                                              "channel map",
                                                              "The channel map",
                                                              GVC_TYPE_CHANNEL_MAP,
                                                              G_PARAM_READWRITE));

        g_type_class_add_private (klass, sizeof (GvcBalanceBarPrivate));
}


static gboolean
on_left (pa_channel_position_t p)
{
    return
        p == PA_CHANNEL_POSITION_FRONT_LEFT ||
        p == PA_CHANNEL_POSITION_REAR_LEFT ||
        p == PA_CHANNEL_POSITION_FRONT_LEFT_OF_CENTER ||
        p == PA_CHANNEL_POSITION_SIDE_LEFT ||
        p == PA_CHANNEL_POSITION_TOP_FRONT_LEFT ||
        p == PA_CHANNEL_POSITION_TOP_REAR_LEFT;
}

static gboolean
on_right (pa_channel_position_t p)
{
    return
        p == PA_CHANNEL_POSITION_FRONT_RIGHT ||
        p == PA_CHANNEL_POSITION_REAR_RIGHT ||
        p == PA_CHANNEL_POSITION_FRONT_RIGHT_OF_CENTER ||
        p == PA_CHANNEL_POSITION_SIDE_RIGHT ||
        p == PA_CHANNEL_POSITION_TOP_FRONT_RIGHT ||
        p == PA_CHANNEL_POSITION_TOP_REAR_RIGHT;
}

static void
on_adjustment_value_changed (GtkAdjustment *adjustment,
                             GvcBalanceBar *bar)
{
        gdouble                val;
        gdouble               *gains;
        pa_channel_position_t *positions;
        guint                  num_channels;
        guint                  i;
        gdouble                left_v;
        gdouble                center_v;
        gdouble                right_v;

        val = gtk_adjustment_get_value (adjustment);

        if (bar->priv->channel_map == NULL) {
                return;
        }

        left_v = 1.0;
        right_v = 1.0;

        /* FIXME: handle RTOL locales */
        if (val > 0) {
                left_v = 1.0 - val;
        } else if (val < 0) {
                right_v = 1.0 - ABS(val);
        }
        center_v = (left_v + right_v) / 2.0;

        num_channels = gvc_channel_map_get_num_channels (bar->priv->channel_map);
        positions = gvc_channel_map_get_positions (bar->priv->channel_map);
        gains = gvc_channel_map_get_gains (bar->priv->channel_map);

        for (i = 0; i < num_channels; i++) {
                if (on_left (positions[i])) {
                        gains[i] = left_v;
                } else if (on_right (positions[i])) {
                        gains[i] = right_v;
                } else {
                        gains[i] = center_v;
                }
        }

        gvc_channel_map_gains_changed (bar->priv->channel_map);
}

static void
gvc_balance_bar_init (GvcBalanceBar *bar)
{
        GtkWidget *frame;

        bar->priv = GVC_BALANCE_BAR_GET_PRIVATE (bar);

        bar->priv->adjustment = GTK_ADJUSTMENT (gtk_adjustment_new (0.0,
                                                                    -1.0,
                                                                    1.0,
                                                                    0.05,
                                                                    0.1,
                                                                    0.1));
        g_object_ref_sink (bar->priv->adjustment);
        g_signal_connect (bar->priv->adjustment,
                          "value-changed",
                          G_CALLBACK (on_adjustment_value_changed),
                          bar);

        bar->priv->label = gtk_label_new_with_mnemonic (_("_Balance:"));
        gtk_misc_set_alignment (GTK_MISC (bar->priv->label),
                                0.0,
                                0.5);
        /* frame */
        frame = gtk_frame_new (NULL);
        gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);
        gtk_container_add (GTK_CONTAINER (bar), frame);

        /* box with scale */
        bar->priv->scale_box = _scale_box_new (bar);
        gtk_container_add (GTK_CONTAINER (frame), bar->priv->scale_box);
        gtk_widget_show_all (frame);

        gtk_widget_set_direction (bar->priv->scale, GTK_TEXT_DIR_LTR);
        gtk_label_set_mnemonic_widget (GTK_LABEL (bar->priv->label),
                                       bar->priv->scale);
}

static void
gvc_balance_bar_finalize (GObject *object)
{
        GvcBalanceBar *balance_bar;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GVC_IS_BALANCE_BAR (object));

        balance_bar = GVC_BALANCE_BAR (object);

        g_return_if_fail (balance_bar->priv != NULL);

        if (balance_bar->priv->channel_map != NULL) {
                g_object_unref (balance_bar->priv->channel_map);
        }

        G_OBJECT_CLASS (gvc_balance_bar_parent_class)->finalize (object);
}

GtkWidget *
gvc_balance_bar_new (GvcChannelMap *channel_map)
{
        GObject *bar;
        bar = g_object_new (GVC_TYPE_BALANCE_BAR,
                            "channel-map", channel_map,
                            NULL);
        return GTK_WIDGET (bar);
}

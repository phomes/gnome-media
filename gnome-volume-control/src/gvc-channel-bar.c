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

#define SCALE_SIZE 128

#define GVC_CHANNEL_BAR_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GVC_TYPE_CHANNEL_BAR, GvcChannelBarPrivate))

struct GvcChannelBarPrivate
{
        GtkOrientation orientation;
        GtkWidget     *scale_box;
        GtkWidget     *image;
        GtkWidget     *label;
        GtkWidget     *low_image;
        GtkWidget     *scale;
        GtkWidget     *high_image;
        GtkWidget     *mute_box;
        GtkWidget     *mute_button;
        GtkAdjustment *adjustment;
        GtkAdjustment *zero_adjustment;
        gboolean       show_mute;
        gboolean       is_muted;
        char          *name;
        char          *icon_name;
        GtkSizeGroup  *size_group;
};

enum
{
        PROP_0,
        PROP_ORIENTATION,
        PROP_SHOW_MUTE,
        PROP_IS_MUTED,
        PROP_ADJUSTMENT,
        PROP_NAME,
        PROP_ICON_NAME,
};

static void     gvc_channel_bar_class_init (GvcChannelBarClass *klass);
static void     gvc_channel_bar_init       (GvcChannelBar      *channel_bar);
static void     gvc_channel_bar_finalize   (GObject            *object);

G_DEFINE_TYPE (GvcChannelBar, gvc_channel_bar, GTK_TYPE_HBOX)

static GtkWidget *
_scale_box_new (GvcChannelBar *bar)
{
        GvcChannelBarPrivate *priv = bar->priv;
        GtkWidget            *box;

        if (priv->orientation == GTK_ORIENTATION_VERTICAL) {
                bar->priv->scale_box = box = gtk_vbox_new (FALSE, 6);

                priv->scale = gtk_vscale_new (priv->adjustment);

                gtk_widget_set_size_request (priv->scale, -1, SCALE_SIZE);
                gtk_range_set_inverted (GTK_RANGE (priv->scale), TRUE);

                gtk_box_pack_start (GTK_BOX (box), priv->image, FALSE, FALSE, 0);
                gtk_box_pack_start (GTK_BOX (box), priv->label, FALSE, FALSE, 0);

                gtk_box_pack_start (GTK_BOX (box), priv->high_image, FALSE, FALSE, 0);
                gtk_widget_hide (priv->high_image);
                gtk_box_pack_start (GTK_BOX (box), priv->scale, TRUE, TRUE, 0);
                gtk_box_pack_start (GTK_BOX (box), priv->low_image, FALSE, FALSE, 0);
                gtk_widget_hide (priv->low_image);

                gtk_box_pack_start (GTK_BOX (box), priv->mute_box, FALSE, FALSE, 0);
        } else {
                bar->priv->scale_box = box = gtk_hbox_new (FALSE, 6);

                priv->scale = gtk_hscale_new (priv->adjustment);

                gtk_widget_set_size_request (priv->scale, SCALE_SIZE, -1);

                gtk_box_pack_start (GTK_BOX (box), priv->image, FALSE, FALSE, 0);
                gtk_box_pack_start (GTK_BOX (box), priv->label, FALSE, FALSE, 0);

                gtk_box_pack_start (GTK_BOX (box), priv->low_image, FALSE, FALSE, 0);
                gtk_widget_show (priv->low_image);

                gtk_box_pack_start (GTK_BOX (box), priv->scale, TRUE, TRUE, 0);
                gtk_box_pack_start (GTK_BOX (box), priv->high_image, FALSE, FALSE, 0);
                gtk_widget_show (priv->high_image);
                gtk_box_pack_start (GTK_BOX (box), priv->mute_box, FALSE, FALSE, 0);
        }

        gtk_range_set_update_policy (GTK_RANGE (priv->scale), GTK_UPDATE_DISCONTINUOUS);

        if (bar->priv->size_group != NULL) {
                gtk_size_group_add_widget (bar->priv->size_group, bar->priv->label);
        }

        gtk_scale_set_draw_value (GTK_SCALE (priv->scale), FALSE);

        return box;
}

static void
update_image (GvcChannelBar *bar)
{
        gtk_image_set_from_icon_name (GTK_IMAGE (bar->priv->image),
                                      bar->priv->icon_name,
                                      GTK_ICON_SIZE_DIALOG);

        if (bar->priv->icon_name != NULL) {
                gtk_widget_show (bar->priv->image);
        } else {
                gtk_widget_hide (bar->priv->image);
        }
}

static void
update_label (GvcChannelBar *bar)
{
        gtk_label_set_text (GTK_LABEL (bar->priv->label),
                            bar->priv->name);

        if (bar->priv->name != NULL) {
                gtk_widget_show (bar->priv->label);
        } else {
                gtk_widget_hide (bar->priv->label);
        }
}

void
gvc_channel_bar_set_size_group (GvcChannelBar *bar,
                                GtkSizeGroup  *group)
{
        g_return_if_fail (GVC_IS_CHANNEL_BAR (bar));

        bar->priv->size_group = group;
        gtk_size_group_add_widget (group, bar->priv->scale);
}

void
gvc_channel_bar_set_name (GvcChannelBar  *bar,
                          const char     *name)
{
        g_return_if_fail (GVC_IS_CHANNEL_BAR (bar));

        g_free (bar->priv->name);
        bar->priv->name = g_strdup (name);
        update_label (bar);
        g_object_notify (G_OBJECT (bar), "name");
}

void
gvc_channel_bar_set_icon_name (GvcChannelBar  *bar,
                               const char     *name)
{
        g_return_if_fail (GVC_IS_CHANNEL_BAR (bar));

        g_free (bar->priv->icon_name);
        bar->priv->icon_name = g_strdup (name);
        update_image (bar);
        g_object_notify (G_OBJECT (bar), "icon-name");
}

void
gvc_channel_bar_set_orientation (GvcChannelBar  *bar,
                                 GtkOrientation  orientation)
{
        g_return_if_fail (GVC_IS_CHANNEL_BAR (bar));

        if (orientation != bar->priv->orientation) {
                bar->priv->orientation = orientation;

                if (bar->priv->scale != NULL) {
                        GtkWidget *box;
                        GtkWidget *frame;

                        box = bar->priv->scale_box;
                        frame = box->parent;

                        g_object_ref (bar->priv->image);
                        g_object_ref (bar->priv->label);
                        g_object_ref (bar->priv->mute_box);
                        g_object_ref (bar->priv->low_image);
                        g_object_ref (bar->priv->high_image);

                        gtk_container_remove (GTK_CONTAINER (box), bar->priv->image);
                        gtk_container_remove (GTK_CONTAINER (box), bar->priv->label);
                        gtk_container_remove (GTK_CONTAINER (box), bar->priv->mute_box);
                        gtk_container_remove (GTK_CONTAINER (box), bar->priv->low_image);
                        gtk_container_remove (GTK_CONTAINER (box), bar->priv->scale);
                        gtk_container_remove (GTK_CONTAINER (box), bar->priv->high_image);
                        gtk_container_remove (GTK_CONTAINER (frame), box);

                        bar->priv->scale_box = _scale_box_new (bar);
                        gtk_container_add (GTK_CONTAINER (frame), bar->priv->scale_box);

                        g_object_unref (bar->priv->image);
                        g_object_unref (bar->priv->label);
                        g_object_unref (bar->priv->mute_box);
                        g_object_unref (bar->priv->low_image);
                        g_object_unref (bar->priv->high_image);

                        gtk_widget_show_all (frame);
                }

                g_object_notify (G_OBJECT (bar), "orientation");
        }
}

static void
gvc_channel_bar_set_adjustment (GvcChannelBar *bar,
                                GtkAdjustment *adjustment)
{
        g_return_if_fail (GVC_CHANNEL_BAR (bar));
        g_return_if_fail (GTK_IS_ADJUSTMENT (adjustment));

        if (bar->priv->adjustment != NULL) {
                g_object_unref (bar->priv->adjustment);
        }
        bar->priv->adjustment = g_object_ref_sink (adjustment);

        if (bar->priv->scale != NULL) {
                gtk_range_set_adjustment (GTK_RANGE (bar->priv->scale), adjustment);
        }

        g_object_notify (G_OBJECT (bar), "adjustment");
}

GtkAdjustment *
gvc_channel_bar_get_adjustment (GvcChannelBar *bar)
{
        g_return_val_if_fail (GVC_IS_CHANNEL_BAR (bar), NULL);

        return bar->priv->adjustment;
}

static void
on_zero_adjustment_value_changed (GtkAdjustment *adjustment,
                                  GvcChannelBar *bar)
{
        gdouble value;

        value = gtk_adjustment_get_value (bar->priv->zero_adjustment);
        gtk_adjustment_set_value (bar->priv->adjustment, value);

        /* this means the adjustment moved away from zero and
          therefore we should unmute and set the volume. */

        gvc_channel_bar_set_is_muted (bar, FALSE);
}

static void
update_mute_button (GvcChannelBar *bar)
{
        if (bar->priv->show_mute) {
                gtk_widget_show (bar->priv->mute_button);
                gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (bar->priv->mute_button),
                                              bar->priv->is_muted);
        } else {
                gtk_widget_hide (bar->priv->mute_button);

                if (bar->priv->is_muted) {
                        /* If we aren't showing the mute button then
                         * move slider to the zero.  But we don't want to
                         * change the adjustment.  */
                        g_signal_handlers_block_by_func (bar->priv->zero_adjustment,
                                                         on_zero_adjustment_value_changed,
                                                         bar);
                        gtk_adjustment_set_value (bar->priv->zero_adjustment, 0);
                        g_signal_handlers_unblock_by_func (bar->priv->zero_adjustment,
                                                           on_zero_adjustment_value_changed,
                                                           bar);
                        gtk_range_set_adjustment (GTK_RANGE (bar->priv->scale),
                                                  bar->priv->zero_adjustment);
                } else {
                        /* no longer muted so restore the original adjustment */
                        gtk_range_set_adjustment (GTK_RANGE (bar->priv->scale),
                                                  bar->priv->adjustment);
                }
        }
}

void
gvc_channel_bar_set_is_muted (GvcChannelBar *bar,
                              gboolean       is_muted)
{
        g_return_if_fail (GVC_IS_CHANNEL_BAR (bar));

        if (is_muted != bar->priv->is_muted) {
                bar->priv->is_muted = is_muted;
                g_object_notify (G_OBJECT (bar), "is-muted");
                update_mute_button (bar);
        }
}

gboolean
gvc_channel_bar_get_is_muted  (GvcChannelBar *bar)
{
        g_return_val_if_fail (GVC_IS_CHANNEL_BAR (bar), FALSE);
        return bar->priv->is_muted;
}

void
gvc_channel_bar_set_show_mute (GvcChannelBar *bar,
                               gboolean       show_mute)
{
        g_return_if_fail (GVC_IS_CHANNEL_BAR (bar));

        if (show_mute != bar->priv->show_mute) {
                bar->priv->show_mute = show_mute;
                g_object_notify (G_OBJECT (bar), "show-mute");
                update_mute_button (bar);
        }
}

gboolean
gvc_channel_bar_get_show_mute (GvcChannelBar *bar)
{
        g_return_val_if_fail (GVC_IS_CHANNEL_BAR (bar), FALSE);
        return bar->priv->show_mute;
}

static void
gvc_channel_bar_set_property (GObject       *object,
                              guint          prop_id,
                              const GValue  *value,
                              GParamSpec    *pspec)
{
        GvcChannelBar *self = GVC_CHANNEL_BAR (object);

        switch (prop_id) {
        case PROP_ORIENTATION:
                gvc_channel_bar_set_orientation (self, g_value_get_enum (value));
                break;
        case PROP_IS_MUTED:
                gvc_channel_bar_set_is_muted (self, g_value_get_boolean (value));
                break;
        case PROP_SHOW_MUTE:
                gvc_channel_bar_set_show_mute (self, g_value_get_boolean (value));
                break;
        case PROP_NAME:
                gvc_channel_bar_set_name (self, g_value_get_string (value));
                break;
        case PROP_ICON_NAME:
                gvc_channel_bar_set_icon_name (self, g_value_get_string (value));
                break;
        case PROP_ADJUSTMENT:
                gvc_channel_bar_set_adjustment (self, g_value_get_object (value));
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
gvc_channel_bar_get_property (GObject     *object,
                              guint        prop_id,
                              GValue      *value,
                              GParamSpec  *pspec)
{
        GvcChannelBar *self = GVC_CHANNEL_BAR (object);
        GvcChannelBarPrivate *priv = self->priv;

        switch (prop_id) {
        case PROP_ORIENTATION:
                g_value_set_enum (value, priv->orientation);
                break;
        case PROP_IS_MUTED:
                g_value_set_boolean (value, priv->is_muted);
                break;
        case PROP_SHOW_MUTE:
                g_value_set_boolean (value, priv->show_mute);
                break;
        case PROP_NAME:
                g_value_set_string (value, priv->name);
                break;
        case PROP_ICON_NAME:
                g_value_set_string (value, priv->icon_name);
                break;
        case PROP_ADJUSTMENT:
                g_value_set_object (value, gvc_channel_bar_get_adjustment (self));
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static GObject *
gvc_channel_bar_constructor (GType                  type,
                             guint                  n_construct_properties,
                             GObjectConstructParam *construct_params)
{
        GObject       *object;
        GvcChannelBar *self;
        GtkWidget     *frame;

        object = G_OBJECT_CLASS (gvc_channel_bar_parent_class)->constructor (type, n_construct_properties, construct_params);

        self = GVC_CHANNEL_BAR (object);

        /* frame */
        frame = gtk_frame_new (NULL);
        gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_NONE);
        gtk_container_add (GTK_CONTAINER (self), frame);

        /* box with scale */
        self->priv->scale_box = _scale_box_new (self);
        gtk_container_add (GTK_CONTAINER (frame), self->priv->scale_box);

        update_mute_button (self);

        gtk_widget_show_all (frame);

        return object;
}

static void
gvc_channel_bar_class_init (GvcChannelBarClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);

        object_class->constructor = gvc_channel_bar_constructor;
        object_class->finalize = gvc_channel_bar_finalize;
        object_class->set_property = gvc_channel_bar_set_property;
        object_class->get_property = gvc_channel_bar_get_property;

        g_object_class_install_property (object_class,
                                         PROP_ORIENTATION,
                                         g_param_spec_enum ("orientation",
                                                            "Orientation",
                                                            "The orientation of the scale",
                                                            GTK_TYPE_ORIENTATION,
                                                            GTK_ORIENTATION_VERTICAL,
                                                            G_PARAM_READWRITE));
        g_object_class_install_property (object_class,
                                         PROP_IS_MUTED,
                                         g_param_spec_boolean ("is-muted",
                                                               "is muted",
                                                               "Whether stream is muted",
                                                               FALSE,
                                                               G_PARAM_READWRITE|G_PARAM_CONSTRUCT));
        g_object_class_install_property (object_class,
                                         PROP_SHOW_MUTE,
                                         g_param_spec_boolean ("show-mute",
                                                               "show mute",
                                                               "Whether stream is muted",
                                                               FALSE,
                                                               G_PARAM_READWRITE|G_PARAM_CONSTRUCT));

        g_object_class_install_property (object_class,
                                         PROP_ADJUSTMENT,
                                         g_param_spec_object ("adjustment",
                                                              "Adjustment",
                                                              "The GtkAdjustment that contains the current value of this scale button object",
                                                              GTK_TYPE_ADJUSTMENT,
                                                              G_PARAM_READWRITE));
        g_object_class_install_property (object_class,
                                         PROP_NAME,
                                         g_param_spec_string ("name",
                                                              "Name",
                                                              "Name to display for this stream",
                                                              NULL,
                                                              G_PARAM_READWRITE|G_PARAM_CONSTRUCT));
        g_object_class_install_property (object_class,
                                         PROP_ICON_NAME,
                                         g_param_spec_string ("icon-name",
                                                              "Icon Name",
                                                              "Name of icon to display for this stream",
                                                              NULL,
                                                              G_PARAM_READWRITE|G_PARAM_CONSTRUCT));

        g_type_class_add_private (klass, sizeof (GvcChannelBarPrivate));
}

static void
on_mute_button_toggled (GtkToggleButton *button,
                        GvcChannelBar   *bar)
{
        gboolean is_muted;
        is_muted = gtk_toggle_button_get_active (button);
        gvc_channel_bar_set_is_muted (bar, is_muted);
}

static void
gvc_channel_bar_init (GvcChannelBar *bar)
{
        bar->priv = GVC_CHANNEL_BAR_GET_PRIVATE (bar);

        bar->priv->orientation = GTK_ORIENTATION_VERTICAL;
        bar->priv->adjustment = GTK_ADJUSTMENT (gtk_adjustment_new (0.0,
                                                                    0.0,
                                                                    65536.0,
                                                                    65536.0/100.0,
                                                                    65536.0/10.0,
                                                                    0.0));
        g_object_ref_sink (bar->priv->adjustment);

        bar->priv->zero_adjustment = GTK_ADJUSTMENT (gtk_adjustment_new (0.0,
                                                                         0.0,
                                                                         65536.0,
                                                                         65536.0/100.0,
                                                                         65536.0/10.0,
                                                                         0.0));
        g_object_ref_sink (bar->priv->zero_adjustment);

        g_signal_connect (bar->priv->zero_adjustment,
                          "value-changed",
                          G_CALLBACK (on_zero_adjustment_value_changed),
                          bar);

        bar->priv->mute_button = gtk_check_button_new_with_label (_("Mute"));
        gtk_widget_set_no_show_all (bar->priv->mute_button, TRUE);
        g_signal_connect (bar->priv->mute_button,
                          "toggled",
                          G_CALLBACK (on_mute_button_toggled),
                          bar);
        bar->priv->mute_box = gtk_alignment_new (0.5, 0.5, 0, 0);
        gtk_container_add (GTK_CONTAINER (bar->priv->mute_box), bar->priv->mute_button);

        bar->priv->low_image = gtk_image_new_from_icon_name ("audio-volume-low",
                                                             GTK_ICON_SIZE_BUTTON);
        gtk_widget_set_no_show_all (bar->priv->low_image, TRUE);
        bar->priv->high_image = gtk_image_new_from_icon_name ("audio-volume-high",
                                                              GTK_ICON_SIZE_BUTTON);
        gtk_widget_set_no_show_all (bar->priv->high_image, TRUE);

        bar->priv->image = gtk_image_new ();
        gtk_widget_set_no_show_all (bar->priv->image, TRUE);

        bar->priv->label = gtk_label_new (NULL);
        gtk_widget_set_no_show_all (bar->priv->label, TRUE);
}

static void
gvc_channel_bar_finalize (GObject *object)
{
        GvcChannelBar *channel_bar;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GVC_IS_CHANNEL_BAR (object));

        channel_bar = GVC_CHANNEL_BAR (object);

        g_return_if_fail (channel_bar->priv != NULL);

        g_free (channel_bar->priv->name);
        g_free (channel_bar->priv->icon_name);

        G_OBJECT_CLASS (gvc_channel_bar_parent_class)->finalize (object);
}

GtkWidget *
gvc_channel_bar_new (void)
{
        GObject *bar;
        bar = g_object_new (GVC_TYPE_CHANNEL_BAR,
                            NULL);
        return GTK_WIDGET (bar);
}

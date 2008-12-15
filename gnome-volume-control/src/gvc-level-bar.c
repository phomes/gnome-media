/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 William Jon McCann <william.jon.mccann@gmail.com>
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
#include <math.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gtk/gtk.h>

#include "gvc-level-bar.h"

#define NUM_BOXES 15

#define GVC_LEVEL_BAR_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GVC_TYPE_LEVEL_BAR, GvcLevelBarPrivate))

#define MIN_HORIZONTAL_BAR_WIDTH   150
#define HORIZONTAL_BAR_HEIGHT      6
#define VERTICAL_BAR_WIDTH         6
#define MIN_VERTICAL_BAR_HEIGHT    400

struct GvcLevelBarPrivate
{
        GtkOrientation orientation;
        GtkAdjustment *peak_adjustment;
        GtkAdjustment *rms_adjustment;
        int            scale;
        gdouble        peak_fraction;
        gdouble        rms_fraction;
        gdouble        max_peak;
        guint          max_peak_id;
};

enum
{
        PROP_0,
        PROP_PEAK_ADJUSTMENT,
        PROP_RMS_ADJUSTMENT,
        PROP_SCALE,
        PROP_ORIENTATION,
};

static void     gvc_level_bar_class_init (GvcLevelBarClass *klass);
static void     gvc_level_bar_init       (GvcLevelBar      *level_bar);
static void     gvc_level_bar_finalize   (GObject            *object);

G_DEFINE_TYPE (GvcLevelBar, gvc_level_bar, GTK_TYPE_HBOX)

static gdouble
fraction_from_adjustment (GvcLevelBar   *bar,
                          GtkAdjustment *adjustment)
{
        gdouble level;
        gdouble fraction;
        gdouble min;
        gdouble max;

        level = gtk_adjustment_get_value (adjustment);

        min = gtk_adjustment_get_lower (adjustment);
        max = gtk_adjustment_get_upper (adjustment);

        switch (bar->priv->scale) {
        case GVC_LEVEL_SCALE_LINEAR:
                fraction = (level - min) / (max - min);
                break;
        case GVC_LEVEL_SCALE_LOG:
                fraction = log10 ((level - min + 1) / (max - min + 1));
                break;
        default:
                g_assert_not_reached ();
        }

        return fraction;
}

static gboolean
reset_max_peak (GvcLevelBar *bar)
{
        gdouble min;

        min = gtk_adjustment_get_lower (bar->priv->peak_adjustment);
        bar->priv->max_peak = min;
        bar->priv->max_peak_id = 0;
        return FALSE;
}

static void
update_peak_value (GvcLevelBar *bar)
{
        gdouble val;

        val = fraction_from_adjustment (bar, bar->priv->peak_adjustment);
        bar->priv->peak_fraction = val;
        if (val > bar->priv->max_peak) {
                if (bar->priv->max_peak_id > 0) {
                        g_source_remove (bar->priv->max_peak_id);
                }
                bar->priv->max_peak_id = g_timeout_add_seconds (1, (GSourceFunc)reset_max_peak, bar);
                bar->priv->max_peak = val;
        }
        gtk_widget_queue_draw (GTK_WIDGET (bar));
}

static void
update_rms_value (GvcLevelBar *bar)
{
        gdouble val;

        val = fraction_from_adjustment (bar, bar->priv->rms_adjustment);
        bar->priv->rms_fraction = val;
        gtk_widget_queue_draw (GTK_WIDGET (bar));
}

GtkOrientation
gvc_level_bar_get_orientation (GvcLevelBar *bar)
{
        g_return_val_if_fail (GVC_IS_LEVEL_BAR (bar), 0);
        return bar->priv->orientation;
}

void
gvc_level_bar_set_orientation (GvcLevelBar   *bar,
                               GtkOrientation orientation)
{
        g_return_if_fail (GVC_IS_LEVEL_BAR (bar));

        if (orientation != bar->priv->orientation) {
                bar->priv->orientation = orientation;
                gtk_widget_queue_draw (GTK_WIDGET (bar));
                g_object_notify (G_OBJECT (bar), "orientation");
        }
}

static void
on_peak_adjustment_value_changed (GtkAdjustment *adjustment,
                                  GvcLevelBar   *bar)
{
        update_peak_value (bar);
}

static void
on_rms_adjustment_value_changed (GtkAdjustment *adjustment,
                                 GvcLevelBar   *bar)
{
        update_rms_value (bar);
}

void
gvc_level_bar_set_peak_adjustment (GvcLevelBar   *bar,
                                   GtkAdjustment *adjustment)
{
        g_return_if_fail (GVC_LEVEL_BAR (bar));
        g_return_if_fail (GTK_IS_ADJUSTMENT (adjustment));

        if (bar->priv->peak_adjustment != NULL) {
                g_signal_handlers_disconnect_by_func (bar->priv->peak_adjustment,
                                                      G_CALLBACK (on_peak_adjustment_value_changed),
                                                      bar);
                g_object_unref (bar->priv->peak_adjustment);
        }

        bar->priv->peak_adjustment = g_object_ref_sink (adjustment);

        g_signal_connect (bar->priv->peak_adjustment,
                          "value-changed",
                          G_CALLBACK (on_peak_adjustment_value_changed),
                          bar);

        update_peak_value (bar);

        g_object_notify (G_OBJECT (bar), "peak-adjustment");
}

void
gvc_level_bar_set_rms_adjustment (GvcLevelBar   *bar,
                                  GtkAdjustment *adjustment)
{
        g_return_if_fail (GVC_LEVEL_BAR (bar));
        g_return_if_fail (GTK_IS_ADJUSTMENT (adjustment));

        if (bar->priv->rms_adjustment != NULL) {
                g_signal_handlers_disconnect_by_func (bar->priv->peak_adjustment,
                                                      G_CALLBACK (on_rms_adjustment_value_changed),
                                                      bar);
                g_object_unref (bar->priv->rms_adjustment);
        }

        bar->priv->rms_adjustment = g_object_ref_sink (adjustment);


        g_signal_connect (bar->priv->peak_adjustment,
                          "value-changed",
                          G_CALLBACK (on_peak_adjustment_value_changed),
                          bar);

        update_rms_value (bar);

        g_object_notify (G_OBJECT (bar), "rms-adjustment");
}

GtkAdjustment *
gvc_level_bar_get_peak_adjustment (GvcLevelBar *bar)
{
        g_return_val_if_fail (GVC_IS_LEVEL_BAR (bar), NULL);

        return bar->priv->peak_adjustment;
}

GtkAdjustment *
gvc_level_bar_get_rms_adjustment (GvcLevelBar *bar)
{
        g_return_val_if_fail (GVC_IS_LEVEL_BAR (bar), NULL);

        return bar->priv->rms_adjustment;
}

void
gvc_level_bar_set_scale (GvcLevelBar  *bar,
                         GvcLevelScale scale)
{
        g_return_if_fail (GVC_IS_LEVEL_BAR (bar));

        if (scale != bar->priv->scale) {
                bar->priv->scale = scale;

                update_peak_value (bar);
                update_rms_value (bar);

                g_object_notify (G_OBJECT (bar), "scale");
        }
}

static void
gvc_level_bar_set_property (GObject       *object,
                              guint          prop_id,
                              const GValue  *value,
                              GParamSpec    *pspec)
{
        GvcLevelBar *self = GVC_LEVEL_BAR (object);

        switch (prop_id) {
        case PROP_SCALE:
                gvc_level_bar_set_scale (self, g_value_get_int (value));
                break;
        case PROP_ORIENTATION:
                gvc_level_bar_set_orientation (self, g_value_get_enum (value));
                break;
        case PROP_PEAK_ADJUSTMENT:
                gvc_level_bar_set_peak_adjustment (self, g_value_get_object (value));
                break;
        case PROP_RMS_ADJUSTMENT:
                gvc_level_bar_set_rms_adjustment (self, g_value_get_object (value));
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static void
gvc_level_bar_get_property (GObject     *object,
                              guint        prop_id,
                              GValue      *value,
                              GParamSpec  *pspec)
{
        GvcLevelBar *self = GVC_LEVEL_BAR (object);

        switch (prop_id) {
        case PROP_SCALE:
                g_value_set_int (value, self->priv->scale);
                break;
        case PROP_ORIENTATION:
                g_value_set_enum (value, self->priv->orientation);
                break;
        case PROP_PEAK_ADJUSTMENT:
                g_value_set_object (value, self->priv->peak_adjustment);
                break;
        case PROP_RMS_ADJUSTMENT:
                g_value_set_object (value, self->priv->rms_adjustment);
                break;
        default:
                G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
                break;
        }
}

static GObject *
gvc_level_bar_constructor (GType                  type,
                           guint                  n_construct_properties,
                           GObjectConstructParam *construct_params)
{
        GObject     *object;
        GvcLevelBar *self;

        object = G_OBJECT_CLASS (gvc_level_bar_parent_class)->constructor (type, n_construct_properties, construct_params);

        self = GVC_LEVEL_BAR (object);

        return object;
}

static void
gvc_level_bar_size_request (GtkWidget      *widget,
                            GtkRequisition *requisition)
{
        GvcLevelBar *bar;

        g_return_if_fail (GVC_IS_LEVEL_BAR (widget));
        g_return_if_fail (requisition != NULL);

        bar = GVC_LEVEL_BAR (widget);

        switch (bar->priv->orientation) {
        case GTK_ORIENTATION_VERTICAL:
                requisition->width = VERTICAL_BAR_WIDTH;
                requisition->height = MIN_VERTICAL_BAR_HEIGHT;
                break;
        case GTK_ORIENTATION_HORIZONTAL:
                requisition->width = MIN_HORIZONTAL_BAR_WIDTH;
                requisition->height = HORIZONTAL_BAR_HEIGHT;
                break;
        default:
                g_assert_not_reached ();
                break;
        }
}

static void
gvc_level_bar_size_allocate (GtkWidget     *widget,
                             GtkAllocation *allocation)
{
        GvcLevelBar *bar;

        g_return_if_fail (GVC_IS_LEVEL_BAR (widget));
        g_return_if_fail (allocation != NULL);

        bar = GVC_LEVEL_BAR (widget);

        /* FIXME: add height property, labels, etc */
        GTK_WIDGET_CLASS (gvc_level_bar_parent_class)->size_allocate (widget, allocation);

        widget->allocation = *allocation;

        if (bar->priv->orientation == GTK_ORIENTATION_VERTICAL) {
                allocation->height = MIN (widget->allocation.height, MIN_VERTICAL_BAR_HEIGHT);
                allocation->width = MAX (widget->allocation.width, VERTICAL_BAR_WIDTH);
        } else {
                allocation->width = MIN (widget->allocation.width, MIN_HORIZONTAL_BAR_WIDTH);
                allocation->height = MAX (widget->allocation.height, HORIZONTAL_BAR_HEIGHT);
        }
}

static void
curved_rectangle (cairo_t *cr,
                  double   x0,
                  double   y0,
                  double   width,
                  double   height,
                  double   radius)
{
        double x1;
        double y1;

        x1 = x0 + width;
        y1 = y0 + height;

        if (!width || !height) {
                return;
        }

        if (width / 2 < radius) {
                if (height / 2 < radius) {
                        cairo_move_to  (cr, x0, (y0 + y1) / 2);
                        cairo_curve_to (cr, x0 ,y0, x0, y0, (x0 + x1) / 2, y0);
                        cairo_curve_to (cr, x1, y0, x1, y0, x1, (y0 + y1) / 2);
                        cairo_curve_to (cr, x1, y1, x1, y1, (x1 + x0) / 2, y1);
                        cairo_curve_to (cr, x0, y1, x0, y1, x0, (y0 + y1) / 2);
                } else {
                        cairo_move_to  (cr, x0, y0 + radius);
                        cairo_curve_to (cr, x0, y0, x0, y0, (x0 + x1) / 2, y0);
                        cairo_curve_to (cr, x1, y0, x1, y0, x1, y0 + radius);
                        cairo_line_to (cr, x1, y1 - radius);
                        cairo_curve_to (cr, x1, y1, x1, y1, (x1 + x0) / 2, y1);
                        cairo_curve_to (cr, x0, y1, x0, y1, x0, y1 - radius);
                }
        } else {
                if (height / 2 < radius) {
                        cairo_move_to  (cr, x0, (y0 + y1) / 2);
                        cairo_curve_to (cr, x0, y0, x0 , y0, x0 + radius, y0);
                        cairo_line_to (cr, x1 - radius, y0);
                        cairo_curve_to (cr, x1, y0, x1, y0, x1, (y0 + y1) / 2);
                        cairo_curve_to (cr, x1, y1, x1, y1, x1 - radius, y1);
                        cairo_line_to (cr, x0 + radius, y1);
                        cairo_curve_to (cr, x0, y1, x0, y1, x0, (y0 + y1) / 2);
                } else {
                        cairo_move_to  (cr, x0, y0 + radius);
                        cairo_curve_to (cr, x0 , y0, x0 , y0, x0 + radius, y0);
                        cairo_line_to (cr, x1 - radius, y0);
                        cairo_curve_to (cr, x1, y0, x1, y0, x1, y0 + radius);
                        cairo_line_to (cr, x1, y1 - radius);
                        cairo_curve_to (cr, x1, y1, x1, y1, x1 - radius, y1);
                        cairo_line_to (cr, x0 + radius, y1);
                        cairo_curve_to (cr, x0, y1, x0, y1, x0, y1 - radius);
                }
        }

        cairo_close_path (cr);
}

static int
gvc_level_bar_expose (GtkWidget      *widget,
                      GdkEventExpose *event)
{
        GvcLevelBar     *bar;
        int              rms_level;
        int              peak_level;
        int              max_peak_level;
        int              width;
        int              height;
        cairo_t         *cr;
        GdkColor         color;
        double           bg_r;
        double           bg_g;
        double           bg_b;
        double           bdr_r;
        double           bdr_g;
        double           bdr_b;
        double           fl_r;
        double           fl_g;
        double           fl_b;

        g_return_val_if_fail (GVC_IS_LEVEL_BAR (widget), FALSE);
        g_return_val_if_fail (event != NULL, FALSE);

        bar = GVC_LEVEL_BAR (widget);

        width = widget->allocation.width - 2;
        height = widget->allocation.height - 2;

        color = widget->style->bg [GTK_STATE_NORMAL];
        bg_r = (float)color.red / 65535.0;
        bg_g = (float)color.green / 65535.0;
        bg_b = (float)color.blue / 65535.0;
        color = widget->style->dark [GTK_STATE_NORMAL];
        bdr_r = (float)color.red / 65535.0;
        bdr_g = (float)color.green / 65535.0;
        bdr_b = (float)color.blue / 65535.0;
        color = widget->style->bg [GTK_STATE_SELECTED];
        fl_r = (float)color.red / 65535.0;
        fl_g = (float)color.green / 65535.0;
        fl_b = (float)color.blue / 65535.0;

        cr = gdk_cairo_create (widget->window);

        cairo_translate (cr,
                         widget->allocation.x,
                         widget->allocation.y);

        if (bar->priv->orientation == GTK_ORIENTATION_VERTICAL) {
                int i;
                int bx;
                int by;
                int bw;
                int bh;
                int br;
                int delta;

                rms_level = bar->priv->rms_fraction * height;
                peak_level = bar->priv->peak_fraction * height;
                max_peak_level = bar->priv->max_peak * height;

                delta = height / NUM_BOXES;
                bx = 0;
                by = 0;
                bh = delta / 2;
                bw = width;
                br = bw / 10;

                for (i = 0; i < NUM_BOXES; i++) {
                        by = i * delta;
                        curved_rectangle (cr, bx, by, bw, bh, br);

                        if ((max_peak_level > by)
                            && (max_peak_level < (by + delta))) {
                                /* fill peak foreground */
                                cairo_set_source_rgb (cr, fl_r, fl_g, fl_b);
                                cairo_fill_preserve (cr);
                        } else if (peak_level > by) {
                                /* fill background */
                                cairo_set_source_rgb (cr, bg_r, bg_g, bg_b);
                                cairo_fill_preserve (cr);
                                /* fill foreground */
                                cairo_set_source_rgba (cr, fl_r, fl_g, fl_b, 0.5);
                                cairo_fill_preserve (cr);
                        } else {
                                /* fill background */
                                cairo_set_source_rgb (cr, bg_r, bg_g, bg_b);
                                cairo_fill_preserve (cr);
                        }

                        /* stroke border */
                        cairo_set_source_rgb (cr, bdr_r, bdr_g, bdr_b);
                        cairo_set_line_width (cr, 1);
                        cairo_stroke (cr);
                }

        } else {
                int i;
                int bx;
                int by;
                int bw;
                int bh;
                int br;
                int delta;

                rms_level = bar->priv->rms_fraction * width;
                peak_level = bar->priv->peak_fraction * width;
                max_peak_level = bar->priv->max_peak * width;

                delta = width / NUM_BOXES;
                bx = 0;
                by = 0;
                bw = delta / 2;
                bh = height;
                br = bh / 10;

                for (i = 0; i < NUM_BOXES; i++) {
                        bx = i * delta;
                        curved_rectangle (cr, bx, by, bw, bh, br);

                        if ((max_peak_level > bx)
                            && (max_peak_level < (bx + delta))) {
                                /* fill peak foreground */
                                cairo_set_source_rgb (cr, fl_r, fl_g, fl_b);
                                cairo_fill_preserve (cr);
                        } else if (peak_level > bx) {
                                /* fill background */
                                cairo_set_source_rgb (cr, bg_r, bg_g, bg_b);
                                cairo_fill_preserve (cr);
                                /* fill foreground */
                                cairo_set_source_rgba (cr, fl_r, fl_g, fl_b, 0.5);
                                cairo_fill_preserve (cr);
                        } else {
                                /* fill background */
                                cairo_set_source_rgb (cr, bg_r, bg_g, bg_b);
                                cairo_fill_preserve (cr);
                        }

                        /* stroke border */
                        cairo_set_source_rgb (cr, bdr_r, bdr_g, bdr_b);
                        cairo_set_line_width (cr, 1);
                        cairo_stroke (cr);
                }
        }
        cairo_destroy (cr);

        return FALSE;
}

static void
gvc_level_bar_class_init (GvcLevelBarClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);
        GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);

        object_class->constructor = gvc_level_bar_constructor;
        object_class->finalize = gvc_level_bar_finalize;
        object_class->set_property = gvc_level_bar_set_property;
        object_class->get_property = gvc_level_bar_get_property;

        widget_class->expose_event = gvc_level_bar_expose;
        widget_class->size_request = gvc_level_bar_size_request;
        widget_class->size_allocate = gvc_level_bar_size_allocate;

        g_object_class_install_property (object_class,
                                         PROP_ORIENTATION,
                                         g_param_spec_enum ("orientation",
                                                            "Orientation",
                                                            "The orientation of the bar",
                                                            GTK_TYPE_ORIENTATION,
                                                            GTK_ORIENTATION_HORIZONTAL,
                                                            G_PARAM_READWRITE));
        g_object_class_install_property (object_class,
                                         PROP_PEAK_ADJUSTMENT,
                                         g_param_spec_object ("peak-adjustment",
                                                              "Peak Adjustment",
                                                              "The GtkAdjustment that contains the current peak value",
                                                              GTK_TYPE_ADJUSTMENT,
                                                              G_PARAM_READWRITE));
        g_object_class_install_property (object_class,
                                         PROP_RMS_ADJUSTMENT,
                                         g_param_spec_object ("rms-adjustment",
                                                              "RMS Adjustment",
                                                              "The GtkAdjustment that contains the current rms value",
                                                              GTK_TYPE_ADJUSTMENT,
                                                              G_PARAM_READWRITE));
        g_object_class_install_property (object_class,
                                         PROP_SCALE,
                                         g_param_spec_int ("scale",
                                                           "Scale",
                                                           "Scale",
                                                           0,
                                                           G_MAXINT,
                                                           GVC_LEVEL_SCALE_LINEAR,
                                                           G_PARAM_READWRITE|G_PARAM_CONSTRUCT));

        g_type_class_add_private (klass, sizeof (GvcLevelBarPrivate));
}

static void
gvc_level_bar_init (GvcLevelBar *bar)
{
        bar->priv = GVC_LEVEL_BAR_GET_PRIVATE (bar);

        bar->priv->peak_adjustment = GTK_ADJUSTMENT (gtk_adjustment_new (0.0,
                                                                         0.0,
                                                                         1.0,
                                                                         0.05,
                                                                         0.1,
                                                                         0.1));
        g_object_ref_sink (bar->priv->peak_adjustment);
        g_signal_connect (bar->priv->peak_adjustment,
                          "value-changed",
                          G_CALLBACK (on_peak_adjustment_value_changed),
                          bar);

        bar->priv->rms_adjustment = GTK_ADJUSTMENT (gtk_adjustment_new (0.0,
                                                                        0.0,
                                                                        1.0,
                                                                        0.05,
                                                                        0.1,
                                                                        0.1));
        g_object_ref_sink (bar->priv->rms_adjustment);
        g_signal_connect (bar->priv->rms_adjustment,
                          "value-changed",
                          G_CALLBACK (on_rms_adjustment_value_changed),
                          bar);

        GTK_WIDGET_SET_FLAGS (GTK_WIDGET (bar), GTK_NO_WINDOW);
}

static void
gvc_level_bar_finalize (GObject *object)
{
        GvcLevelBar *level_bar;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GVC_IS_LEVEL_BAR (object));

        level_bar = GVC_LEVEL_BAR (object);

        g_return_if_fail (level_bar->priv != NULL);

        G_OBJECT_CLASS (gvc_level_bar_parent_class)->finalize (object);
}

GtkWidget *
gvc_level_bar_new (void)
{
        GObject *bar;
        bar = g_object_new (GVC_TYPE_LEVEL_BAR,
                            NULL);
        return GTK_WIDGET (bar);
}

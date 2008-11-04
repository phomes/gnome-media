/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 Red Hat, Inc.
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
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "gvc-applet.h"
#include "gvc-channel-bar.h"
#include "gvc-mixer-control.h"

#define GVC_APPLET_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GVC_TYPE_APPLET, GvcAppletPrivate))

#define SCALE_SIZE 128

static const char *icon_names[] = {
        "audio-volume-muted",
        "audio-volume-low",
        "audio-volume-medium",
        "audio-volume-high",
        NULL
};

struct GvcAppletPrivate
{
        GtkStatusIcon   *status_icon;
        GtkWidget       *dock;
        GtkWidget       *bar;
        GvcMixerControl *control;
        GvcMixerStream  *sink_stream;
        guint            current_icon;
};

static void     gvc_applet_class_init (GvcAppletClass *klass);
static void     gvc_applet_init       (GvcApplet      *applet);
static void     gvc_applet_finalize   (GObject        *object);

G_DEFINE_TYPE (GvcApplet, gvc_applet, G_TYPE_OBJECT)

static void
maybe_show_status_icon (GvcApplet *applet)
{
        gboolean show;

        show = TRUE;

        gtk_status_icon_set_visible (applet->priv->status_icon, show);
}

void
gvc_applet_start (GvcApplet *applet)
{
        g_return_if_fail (GVC_IS_APPLET (applet));

        maybe_show_status_icon (applet);
}

static void
gvc_applet_dispose (GObject *object)
{
        GvcApplet *applet = GVC_APPLET (object);

        if (applet->priv->dock != NULL) {
                gtk_widget_destroy (applet->priv->dock);
                applet->priv->dock = NULL;
        }

        G_OBJECT_CLASS (gvc_applet_parent_class)->dispose (object);
}

static GObject *
gvc_applet_constructor (GType                  type,
                        guint                  n_construct_properties,
                        GObjectConstructParam *construct_params)
{
        GObject   *object;
        GvcApplet *self;

        object = G_OBJECT_CLASS (gvc_applet_parent_class)->constructor (type, n_construct_properties, construct_params);

        self = GVC_APPLET (object);

        gvc_mixer_control_open (self->priv->control);

        return object;
}

static void
gvc_applet_class_init (GvcAppletClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);

        object_class->finalize = gvc_applet_finalize;
        object_class->dispose = gvc_applet_dispose;
        object_class->constructor = gvc_applet_constructor;

        g_type_class_add_private (klass, sizeof (GvcAppletPrivate));
}

static void
on_adjustment_value_changed (GtkAdjustment *adjustment,
                             GvcApplet     *applet)
{
        gdouble volume;

        volume = gtk_adjustment_get_value (adjustment);
        gvc_mixer_stream_change_volume (applet->priv->sink_stream,
                                        (guint)volume);
}

static gboolean
popup_dock (GvcApplet *applet,
            guint      time)
{
        GtkAdjustment *adj;
        GdkRectangle   area;
        GtkOrientation orientation;
        GdkDisplay    *display;
        GdkScreen     *screen;
        gboolean       res;
        int            x, y;

        adj = GTK_ADJUSTMENT (gvc_channel_bar_get_adjustment (GVC_CHANNEL_BAR (applet->priv->bar)));
        gtk_adjustment_set_value (adj,
                                  gvc_mixer_stream_get_volume (applet->priv->sink_stream));

        screen = gtk_status_icon_get_screen (applet->priv->status_icon);
        res = gtk_status_icon_get_geometry (applet->priv->status_icon,
                                            &screen,
                                            &area,
                                            &orientation);
        if (! res) {
                g_warning ("Unable to determine geometry of status icon");
                return FALSE;
        }

        /* position roughly */
        gtk_window_set_screen (GTK_WINDOW (applet->priv->dock), screen);
        x = area.x + area.width;
        y = area.y + area.height;

        if (orientation == GTK_ORIENTATION_VERTICAL) {
                gtk_window_move (GTK_WINDOW (applet->priv->dock), x, area.y);
        } else {
                gtk_window_move (GTK_WINDOW (applet->priv->dock), area.x, y);
        }

        /* FIXME: without this, the popup window appears as a square
         * after changing the orientation
         */
        gtk_window_resize (GTK_WINDOW (applet->priv->dock), 1, 1);

        gtk_widget_show_all (applet->priv->dock);


        /* grab focus */
        gtk_grab_add (applet->priv->dock);

        if (gdk_pointer_grab (applet->priv->dock->window, TRUE,
                              GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
                              GDK_POINTER_MOTION_MASK, NULL, NULL,
                              time)
            != GDK_GRAB_SUCCESS) {
                gtk_grab_remove (applet->priv->dock);
                gtk_widget_hide (applet->priv->dock);
                return FALSE;
        }

        if (gdk_keyboard_grab (applet->priv->dock->window, TRUE, time) != GDK_GRAB_SUCCESS) {
                display = gtk_widget_get_display (applet->priv->dock);
                gdk_display_pointer_ungrab (display, time);
                gtk_grab_remove (applet->priv->dock);
                gtk_widget_hide (applet->priv->dock);
                return FALSE;
        }

        gtk_widget_grab_focus (applet->priv->dock);

        return TRUE;
}

static void
on_status_icon_activate (GtkStatusIcon *status_icon,
                         GvcApplet     *applet)
{
        popup_dock (applet, GDK_CURRENT_TIME);
}

static void
on_menu_activate_open_volume_control (GtkMenuItem *item,
                                      GvcApplet   *applet)
{
        GError *error;

        error = NULL;
        gdk_spawn_command_line_on_screen (gtk_widget_get_screen (applet->priv->dock),
                                          "gnome-volume-control",
                                          &error);

        if (error != NULL) {
                GtkWidget *dialog;

                dialog = gtk_message_dialog_new (NULL,
                                                 0,
                                                 GTK_MESSAGE_ERROR,
                                                 GTK_BUTTONS_CLOSE,
                                                 _("Failed to start Volume Control: %s"),
                                                 error->message);
                g_signal_connect (dialog,
                                  "response",
                                  G_CALLBACK (gtk_widget_destroy),
                                  NULL);
                gtk_widget_show (dialog);
                g_error_free (error);
        }
}

static void
on_status_icon_popup_menu (GtkStatusIcon *status_icon,
                           guint          button,
                           guint          activate_time,
                           GvcApplet     *applet)
{
        GtkWidget *menu;
        GtkWidget *item;
        GtkWidget *image;

        menu = gtk_menu_new ();
        item = gtk_image_menu_item_new_with_mnemonic (_("_Open Volume Control"));
        image = gtk_image_new_from_icon_name ("multimedia-volume-control",
                                              GTK_ICON_SIZE_MENU);
        gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);
        g_signal_connect (item,
                          "activate",
                          G_CALLBACK (on_menu_activate_open_volume_control),
                          applet);
        gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

        gtk_widget_show_all (menu);
        gtk_menu_popup (GTK_MENU (menu),
                        NULL,
                        NULL,
                        gtk_status_icon_position_menu,
                        status_icon,
                        button,
                        activate_time);
}

#if GTK_CHECK_VERSION(2,15,0)
static gboolean
on_status_icon_scroll_event (GtkStatusIcon  *status_icon,
                             GdkEventScroll *event,
                             GvcApplet      *applet)
{
        GtkAdjustment *adj;

        adj = GTK_ADJUSTMENT (gvc_channel_bar_get_adjustment (GVC_CHANNEL_BAR (applet->priv->bar)));

        switch (event->direction) {
        case GDK_SCROLL_UP:
        case GDK_SCROLL_DOWN: {
                gdouble volume;

                volume = gtk_adjustment_get_value (adj);

                if (event->direction == GDK_SCROLL_UP) {
                        volume += adj->step_increment;
                        if (volume > adj->upper) {
                                volume = adj->upper;
                        }
                } else {
                        volume -= adj->step_increment;
                        if (volume < adj->lower) {
                                volume = adj->lower;
                        }
                }

                gtk_adjustment_set_value (adj, volume);
                return TRUE;
        }
        default:
                break;
        }

        return FALSE;
}
#endif

static void
gvc_applet_release_grab (GvcApplet      *applet,
                         GdkEventButton *event)
{
        GdkDisplay     *display;

        /* ungrab focus */
        display = gtk_widget_get_display (GTK_WIDGET (applet->priv->dock));
        gdk_display_keyboard_ungrab (display, event->time);
        gdk_display_pointer_ungrab (display, event->time);
        gtk_grab_remove (applet->priv->dock);

        /* hide again */
        gtk_widget_hide (applet->priv->dock);
}

static gboolean
on_dock_button_press (GtkWidget      *widget,
                      GdkEventButton *event,
                      GvcApplet      *applet)
{
        if (event->type == GDK_BUTTON_PRESS) {
                gvc_applet_release_grab (applet, event);
                return TRUE;
        }

        return FALSE;
}

static void
popdown_dock (GvcApplet *applet)
{
        GdkDisplay *display;

        /* ungrab focus */
        display = gtk_widget_get_display (applet->priv->dock);
        gdk_display_keyboard_ungrab (display, GDK_CURRENT_TIME);
        gdk_display_pointer_ungrab (display, GDK_CURRENT_TIME);
        gtk_grab_remove (applet->priv->dock);

        /* hide again */
        gtk_widget_hide (applet->priv->dock);
}

/* This is called when the grab is broken for
 * either the dock, or the scale itself */
static void
gvc_applet_grab_notify (GvcApplet *applet,
                        gboolean   was_grabbed)
{
        if (was_grabbed != FALSE) {
                return;
        }

        if (!GTK_WIDGET_HAS_GRAB (applet->priv->dock)) {
                return;
        }

        if (gtk_widget_is_ancestor (gtk_grab_get_current (), applet->priv->dock)) {
                return;
        }

        popdown_dock (applet);
}

static void
on_dock_grab_notify (GtkWidget *widget,
                     gboolean   was_grabbed,
                     GvcApplet *applet)
{
        gvc_applet_grab_notify (applet, was_grabbed);
}

static gboolean
on_dock_grab_broken_event (GtkWidget *widget,
                           gboolean   was_grabbed,
                           GvcApplet *applet)
{
        gvc_applet_grab_notify (applet, FALSE);

        return FALSE;
}

static gboolean
on_dock_key_release (GtkWidget   *widget,
                     GdkEventKey *event,
                     GvcApplet   *applet)
{
        if (event->keyval == GDK_Escape) {
                popdown_dock (applet);
                return TRUE;
        }

#if 0
        if (!gtk_bindings_activate_event (GTK_OBJECT (widget), event)) {
                /* The popup hasn't managed the event, pass onto the button */
                gtk_bindings_activate_event (GTK_OBJECT (user_data), event);
        }
#endif
        return TRUE;
}

static void
update_icon (GvcApplet *applet)
{
        guint    volume;
        gboolean is_muted;
        guint    n;

        volume = gvc_mixer_stream_get_volume (applet->priv->sink_stream);
        is_muted = gvc_mixer_stream_get_is_muted (applet->priv->sink_stream);

        /* select image */
        if (volume <= 0 || is_muted) {
                n = 0;
        } else {
                n = 3 * volume / PA_VOLUME_NORM + 1;
                if (n < 1) {
                        n = 1;
                } else if (n > 3) {
                        n = 3;
                }
        }

        /* apparently status icon will reset icon even if
         * if doesn't change */
        if (applet->priv->current_icon != n) {
                gtk_status_icon_set_from_icon_name (GTK_STATUS_ICON (applet->priv->status_icon), icon_names [n]);
                applet->priv->current_icon = n;
        }
}

static void
on_stream_volume_notify (GObject    *object,
                         GParamSpec *pspec,
                         GvcApplet  *applet)
{
        update_icon (applet);
        /* FIXME: update dock too */
}

static void
on_stream_is_muted_notify (GObject    *object,
                           GParamSpec *pspec,
                           GvcApplet  *applet)
{
        update_icon (applet);
        /* FIXME: update dock too */
}

static void
on_control_ready (GvcMixerControl *control,
                  GvcApplet       *applet)
{
        applet->priv->sink_stream = gvc_mixer_control_get_default_sink (control);
        if (applet->priv->sink_stream != NULL) {
                GtkAdjustment *adj;

                adj = GTK_ADJUSTMENT (gvc_channel_bar_get_adjustment (GVC_CHANNEL_BAR (applet->priv->bar)));
                gtk_adjustment_set_value (adj,
                                          gvc_mixer_stream_get_volume (applet->priv->sink_stream));

                g_signal_connect (applet->priv->sink_stream,
                                  "notify::volume",
                                  G_CALLBACK (on_stream_volume_notify),
                                  applet);
                g_signal_connect (applet->priv->sink_stream,
                                  "notify::is-muted",
                                  G_CALLBACK (on_stream_is_muted_notify),
                                  applet);
                update_icon (applet);
        } else {
                g_warning ("Unable to get default sink");
        }
}

static void
on_bar_is_muted_notify (GObject    *object,
                        GParamSpec *pspec,
                        GvcApplet  *applet)
{
        gboolean is_muted;

        is_muted = gvc_channel_bar_get_is_muted (GVC_CHANNEL_BAR (object));
        gvc_mixer_stream_change_is_muted (applet->priv->sink_stream,
                                          is_muted);
}

static void
gvc_applet_init (GvcApplet *applet)
{
        GtkWidget *frame;
        GtkWidget *box;
        GtkAdjustment *adj;

        applet->priv = GVC_APPLET_GET_PRIVATE (applet);

        applet->priv->status_icon = gtk_status_icon_new_from_icon_name (icon_names[0]);
        g_signal_connect (applet->priv->status_icon,
                          "activate",
                          G_CALLBACK (on_status_icon_activate),
                          applet);
        g_signal_connect (applet->priv->status_icon,
                          "popup-menu",
                          G_CALLBACK (on_status_icon_popup_menu),
                          applet);
#if GTK_CHECK_VERSION(2,15,0)
        g_signal_connect (applet->priv->status_icon,
                          "scroll-event",
                          G_CALLBACK (on_status_icon_scroll_event),
                          applet);
#endif

        /* window */
        applet->priv->dock = gtk_window_new (GTK_WINDOW_POPUP);
        gtk_widget_set_name (applet->priv->dock, "gvc-applet-popup-window");
        g_signal_connect (applet->priv->dock,
                          "button-press-event",
                          G_CALLBACK (on_dock_button_press),
                          applet);
        g_signal_connect (applet->priv->dock,
                          "key-release-event",
                          G_CALLBACK (on_dock_key_release),
                          applet);
        g_signal_connect (applet->priv->dock,
                          "grab-notify",
                          G_CALLBACK (on_dock_grab_notify),
                          applet);
        g_signal_connect (applet->priv->dock,
                          "grab-broken-event",
                          G_CALLBACK (on_dock_grab_broken_event),
                          applet);

        gtk_window_set_decorated (GTK_WINDOW (applet->priv->dock), FALSE);

        frame = gtk_frame_new (NULL);
        gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_OUT);
        gtk_container_add (GTK_CONTAINER (applet->priv->dock), frame);
        gtk_widget_show (frame);

        box = gtk_vbox_new (FALSE, 6);
        gtk_container_set_border_width (GTK_CONTAINER (box), 6);
        gtk_container_add (GTK_CONTAINER (frame), box);

        applet->priv->bar = gvc_channel_bar_new ();
        gtk_box_pack_start (GTK_BOX (box), applet->priv->bar, TRUE, FALSE, 0);
        g_signal_connect (applet->priv->bar,
                          "notify::is-muted",
                          G_CALLBACK (on_bar_is_muted_notify),
                          applet);

        applet->priv->control = gvc_mixer_control_new ();
        g_signal_connect (applet->priv->control,
                          "ready",
                          G_CALLBACK (on_control_ready),
                          applet);

        adj = GTK_ADJUSTMENT (gvc_channel_bar_get_adjustment (GVC_CHANNEL_BAR (applet->priv->bar)));
        g_signal_connect (adj,
                          "value-changed",
                          G_CALLBACK (on_adjustment_value_changed),
                          applet);
}

static void
gvc_applet_finalize (GObject *object)
{
        GvcApplet *applet;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GVC_IS_APPLET (object));

        applet = GVC_APPLET (object);

        g_return_if_fail (applet->priv != NULL);
        G_OBJECT_CLASS (gvc_applet_parent_class)->finalize (object);
}

GvcApplet *
gvc_applet_new (void)
{
        GObject *applet;

        applet = g_object_new (GVC_TYPE_APPLET, NULL);

        return GVC_APPLET (applet);
}

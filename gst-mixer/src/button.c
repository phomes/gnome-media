/* GNOME Volume Control
 * Copyright (C) 2003-2004 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *
 * button.c: flat toggle button with icons
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

#include <string.h>
#include <glib.h>
#include <gtk/gtk.h>

#include "button.h"

G_DEFINE_TYPE (GnomeVolumeControlButton, gnome_volume_control_button, GTK_TYPE_BUTTON)


static void	gnome_volume_control_button_class_init	(GnomeVolumeControlButtonClass *klass);
static void	gnome_volume_control_button_init	(GnomeVolumeControlButton *button);
static void	gnome_volume_control_button_dispose	(GObject   *object);

static void	gnome_volume_control_button_clicked	(GtkButton *button);

static gboolean	gnome_volume_control_button_mouseover	(GtkWidget *widget,
							 GdkEventCrossing *event);
static gboolean	gnome_volume_control_button_mouseout	(GtkWidget *widget,
							 GdkEventCrossing *event);


static void
gnome_volume_control_button_class_init (GnomeVolumeControlButtonClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GtkButtonClass *gtkbutton_class = GTK_BUTTON_CLASS (klass);
  GtkWidgetClass *gtkwidget_class = GTK_WIDGET_CLASS (klass);

  gobject_class->dispose = gnome_volume_control_button_dispose;
  gtkbutton_class->clicked = gnome_volume_control_button_clicked;
  gtkwidget_class->enter_notify_event = gnome_volume_control_button_mouseover;
  gtkwidget_class->leave_notify_event = gnome_volume_control_button_mouseout;
}

static void
gnome_volume_control_button_init (GnomeVolumeControlButton *button)
{
  button->active_icon = NULL;
  button->inactive_icon = NULL;

  button->active = FALSE;

  button->status_msg = NULL;
  button->statusbar = NULL;
}

static void
gnome_volume_control_button_dispose (GObject *object)
{
  GnomeVolumeControlButton *button = GNOME_VOLUME_CONTROL_BUTTON (object);

  if (button->status_msg) {
    g_free (button->status_msg);
    button->status_msg = NULL;
  }

  G_OBJECT_CLASS (gnome_volume_control_button_parent_class)->dispose (object);
}

GtkWidget *
gnome_volume_control_button_new (gchar *active_icon,
				 gchar *inactive_icon,
				 GtkStatusbar *statusbar,
				 gchar *status_msg)
{
  GnomeVolumeControlButton *button;
  GtkWidget *image;

  button = g_object_new (GNOME_VOLUME_CONTROL_TYPE_BUTTON, NULL);
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  button->active_icon = active_icon;
  button->inactive_icon = inactive_icon;

  image = gtk_image_new ();
  gtk_container_add (GTK_CONTAINER (button), image);
  gtk_widget_show (image);
  button->image = GTK_IMAGE (image);
  gtk_button_clicked (GTK_BUTTON (button));

  button->statusbar = statusbar;
  button->status_msg = g_strdup (status_msg);

  return GTK_WIDGET (button);
}

gboolean
gnome_volume_control_button_get_active (GnomeVolumeControlButton *button)
{
  return button->active;
}

void
gnome_volume_control_button_set_active (GnomeVolumeControlButton *button,
					gboolean active)
{
  if (button->active != active)
    gtk_button_clicked (GTK_BUTTON (button));
}

static void
gnome_volume_control_button_clicked (GtkButton *_button)
{
  GnomeVolumeControlButton *button = GNOME_VOLUME_CONTROL_BUTTON (_button);

  button->active = !button->active;

  if (strstr (button->active_icon, ".png")) {
    gchar *filename;

    if (button->active)
      filename = g_build_filename (PIX_DIR, button->active_icon, NULL);
    else
      filename = g_build_filename (PIX_DIR, button->inactive_icon, NULL);
   
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file (filename, NULL);
    gtk_image_set_from_pixbuf (button->image, pixbuf);
    g_object_unref (pixbuf);
    g_free (filename);
  } else {
    if (button->active) {
      gtk_image_set_from_icon_name (button->image, button->active_icon,
				GTK_ICON_SIZE_MENU);
    } else {
      gtk_image_set_from_icon_name (button->image, button->inactive_icon,
				GTK_ICON_SIZE_MENU);
    }
  }
}

/*
 * Statusbar stuff.
 */

static gboolean
gnome_volume_control_button_mouseover (GtkWidget *widget,
				       GdkEventCrossing *event)
{
  GnomeVolumeControlButton *button = GNOME_VOLUME_CONTROL_BUTTON (widget);

  gtk_statusbar_push (button->statusbar, 0, button->status_msg);

  return GTK_WIDGET_CLASS (gnome_volume_control_button_parent_class)->enter_notify_event (widget, event);
}

static gboolean
gnome_volume_control_button_mouseout (GtkWidget *widget,
				      GdkEventCrossing *event)
{
  GnomeVolumeControlButton *button = GNOME_VOLUME_CONTROL_BUTTON (widget);

  gtk_statusbar_pop (button->statusbar, 0);

  return GTK_WIDGET_CLASS (gnome_volume_control_button_parent_class)->leave_notify_event (widget, event);
}

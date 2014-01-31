/* GNOME Volume Control
 * Copyright (C) 2003-2004 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *
 * window.h: main window
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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __GVC_WINDOW_H__
#define __GVC_WINDOW_H__

#include <glib.h>
#include <gconf/gconf-client.h>
#include <gst/gst.h>

#include "element.h"

G_BEGIN_DECLS

#define GNOME_VOLUME_CONTROL_TYPE_WINDOW \
  (gnome_volume_control_window_get_type ())
#define GNOME_VOLUME_CONTROL_WINDOW(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GNOME_VOLUME_CONTROL_TYPE_WINDOW, \
			       GnomeVolumeControlWindow))
#define GNOME_VOLUME_CONTROL_WINDOW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), GNOME_VOLUME_CONTROL_TYPE_WINDOW, \
			    GnomeVolumeControlWindowClass))
#define GNOME_VOLUME_CONTROL_IS_WINDOW(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GNOME_VOLUME_CONTROL_TYPE_WINDOW))
#define GNOME_VOLUME_CONTROL_IS_WINDOW_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GNOME_VOLUME_CONTROL_TYPE_WINDOW))

typedef struct {
  GtkWindow parent;

  /* element list */
  GList *elements;

  /* gconf client */
  GConfClient *client;

  /* contents */
  GnomeVolumeControlElement *el;

  /* preferences window, if opened */
  GtkWidget *prefs;

  /* use default mixer */
  gboolean use_default_mixer;
} GnomeVolumeControlWindow;

typedef struct {
  GtkWindowClass klass;
} GnomeVolumeControlWindowClass;

GType		gnome_volume_control_window_get_type		(void);
GtkWidget *	gnome_volume_control_window_new			(GList *elements);
void		gnome_volume_control_window_set_page		(GtkWidget *win, const gchar *page);

G_END_DECLS

#endif /* __GVC_WINDOW_H__ */

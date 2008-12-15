/* GNOME Volume Control
 * Copyright (C) 2003-2004 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *
 * preferences.h: preferences screen
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

#ifndef __GVC_PREFERENCES_H__
#define __GVC_PREFERENCES_H__

#include <glib.h>
#include <gtk/gtk.h>
#include <gconf/gconf-client.h>
#include <gst/interfaces/mixer.h>

G_BEGIN_DECLS

#define GNOME_VOLUME_CONTROL_TYPE_PREFERENCES \
  (gnome_volume_control_preferences_get_type ())
#define GNOME_VOLUME_CONTROL_PREFERENCES(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GNOME_VOLUME_CONTROL_TYPE_PREFERENCES, \
			       GnomeVolumeControlPreferences))
#define GNOME_VOLUME_CONTROL_PREFERENCES_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), GNOME_VOLUME_CONTROL_TYPE_PREFERENCES, \
			    GnomeVolumeControlPreferencesClass))
#define GNOME_VOLUME_CONTROL_IS_PREFERENCES(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GNOME_VOLUME_CONTROL_TYPE_PREFERENCES))
#define GNOME_VOLUME_CONTROL_IS_PREFERENCES_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GNOME_VOLUME_CONTROL_TYPE_PREFERENCES))

typedef struct _GnomeVolumeControlPreferences {
  GtkDialog parent;

  /* current element that we're working on */
  GstMixer *mixer;

  /* gconf client inherited from our parent */
  GConfClient *client;
  guint client_cnxn;

  /* treeview inside us */
  GtkWidget *treeview;
} GnomeVolumeControlPreferences;

typedef struct _GnomeVolumeControlPreferencesClass {
  GtkDialogClass klass;
} GnomeVolumeControlPreferencesClass;

GType	gnome_volume_control_preferences_get_type (void);
GtkWidget *gnome_volume_control_preferences_new	(GstElement  *element,
						 GConfClient *client);
void	gnome_volume_control_preferences_change	(GnomeVolumeControlPreferences *prefs,
						 GstElement  *element);

/*
 * GConf thingy. Escapes spaces and such.
 */
gchar *	get_gconf_key	(GstMixer *mixer, GstMixerTrack *track);


G_END_DECLS

#endif /* __GVC_PREFERENCES_H__ */

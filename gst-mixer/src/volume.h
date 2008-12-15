/* GNOME Volume Control
 * Copyright (C) 2003-2004 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *
 * volume.h: representation of a track's volume channels
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

#ifndef __GVC_VOLUME_H__
#define __GVC_VOLUME_H__

#include <glib.h>
#include <gtk/gtk.h>
#include <gst/interfaces/mixer.h>

G_BEGIN_DECLS

#define GNOME_VOLUME_CONTROL_TYPE_VOLUME \
  (gnome_volume_control_volume_get_type ())
#define GNOME_VOLUME_CONTROL_VOLUME(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GNOME_VOLUME_CONTROL_TYPE_VOLUME, \
			       GnomeVolumeControlVolume))
#define GNOME_VOLUME_CONTROL_VOLUME_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST ((klass), GNOME_VOLUME_CONTROL_TYPE_VOLUME, \
			    GnomeVolumeControlVolumeClass))
#define GNOME_VOLUME_CONTROL_IS_VOLUME(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GNOME_VOLUME_CONTROL_TYPE_VOLUME))
#define GNOME_VOLUME_CONTROL_IS_VOLUME_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE ((klass), GNOME_VOLUME_CONTROL_TYPE_VOLUME))

typedef struct _GnomeVolumeControlVolume {
  GtkFixed parent;

  /* track + mixer */
  GstMixer *mixer;
  GstMixerTrack *track;

  /* padding */
  gint padding;

  /* childs */
  GList *scales;
  GtkWidget *button, *image;

  /* this will be set to true if the user changes volumes
   * in the mixer as a response to a user query. It prevents
   * infinite loops. */
  gboolean locked;

  /* signal ID */
  guint id;
} GnomeVolumeControlVolume;

typedef struct _GnomeVolumeControlVolumeClass {
  GtkFixedClass klass;
} GnomeVolumeControlVolumeClass;

GType		gnome_volume_control_volume_get_type	(void);
GtkWidget *	gnome_volume_control_volume_new	(GstMixer *mixer,
						 GstMixerTrack *track,
						 gint      padding);
void		gnome_volume_control_volume_ask (GnomeVolumeControlVolume *volume,
						 gboolean * real_zero,
						 gboolean * slider_zero);
void		gnome_volume_control_volume_update (GnomeVolumeControlVolume *volume);

G_END_DECLS

#endif /* __GVC_VOLUME_H__ */

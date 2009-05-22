/* GNOME Volume Control
 * Copyright (C) 2003-2004 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *
 * track.h: layout of a single mixer track
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

#ifndef __GVC_TRACK_H__
#define __GVC_TRACK_H__

#include <glib.h>
#include <gtk/gtk.h>
#include <gconf/gconf-client.h>
#include <gst/gst.h>
#include <gst/interfaces/mixer.h>

#include "button.h"

G_BEGIN_DECLS

typedef struct _GnomeVolumeControlTrack {
  /* pointer to table in which we write */
  GtkTable *table;
  gint pos;

  /* gstreamer object pointers */
  GstMixer *mixer;
  GstMixerTrack *track;

  /* widgets associated with this track */
  GtkWidget *label,
	    *image,
	    *sliderbox,
	    *buttonbox,
	    *toggle,
	    *options,
	    *flagbuttonbox;

  GnomeVolumeControlButton *mute, *record;

  /* list of slider adjustments */
  GList *sliders;

  /* separator left/right (or top/bottom) of the actual widget */
  GtkWidget *left_separator,
	    *right_separator;

  /* whether we're currently "visible" */
  gboolean visible;

  /* signal IDs */
  guint id;
} GnomeVolumeControlTrack;

GnomeVolumeControlTrack *
	gnome_volume_control_track_add_playback	(GtkTable *table,
						 gint      tab_pos,
						 GstMixer *mixer,
						 GstMixerTrack *track,
						 GtkWidget *l_sep,
						 GtkWidget *r_sep,
						 GtkWidget *fbox);
GnomeVolumeControlTrack *
	gnome_volume_control_track_add_recording(GtkTable *table,
						 gint      tab_pos,
						 GstMixer *mixer,
						 GstMixerTrack *track,
						 GtkWidget *l_sep,
						 GtkWidget *r_sep,
						 GtkWidget *fbox);

GnomeVolumeControlTrack *
	gnome_volume_control_track_add_switch	(GtkTable *table,
						 gint      tab_pos,
						 GstMixer *mixer,
						 GstMixerTrack *track,
						 GtkWidget *l_sep,
						 GtkWidget *r_sep,
						 GtkWidget *fbox);

GnomeVolumeControlTrack *
	gnome_volume_control_track_add_option	(GtkTable *table,
						 gint      tab_pos,
						 GstMixer *mixer,
						 GstMixerTrack *track,
						 GtkWidget *l_sep,
						 GtkWidget *r_sep,
						 GtkWidget *fbox);

void	gnome_volume_control_track_free		(GnomeVolumeControlTrack *track);

void	gnome_volume_control_track_show		(GnomeVolumeControlTrack *track,
						 gboolean  visible);

void    gnome_volume_control_track_update       (GnomeVolumeControlTrack *trkw);

G_END_DECLS

#endif /* __GVC_TRACK_H__ */

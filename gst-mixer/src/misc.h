/* GNOME Volume Control
 * Copyright (C) 2003-2004 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *
 * element.h: widget representation of a single mixer element
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

#ifndef __GST_MIXER_MISC_H__
#define __GST_MIXER_MISC_H__

#include <glib.h>
#include <gst/interfaces/mixertrack.h>

gint get_page_num (GstMixer *mixer, GstMixerTrack *track);

gchar *get_page_description (gint n);

#endif /* __GST_MIXER_MISC_H__ */

/* GNOME Volume Control
 * Copyright (C) 2003-2004 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *
 * keys.h: GConf key macros
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

#ifndef __GVC_KEYS_H__
#define __GVC_KEYS_H__

G_BEGIN_DECLS

#define GNOME_VOLUME_CONTROL_KEY_DIR \
  "/apps/gnome-volume-control"
#define GNOME_VOLUME_CONTROL_KEY(key) \
  GNOME_VOLUME_CONTROL_KEY_DIR "/" key

#define GNOME_VOLUME_CONTROL_KEY_ACTIVE_ELEMENT \
  GNOME_VOLUME_CONTROL_KEY ("active-element")
#define PREF_UI_WINDOW_WIDTH   GNOME_VOLUME_CONTROL_KEY ("ui/window_width")
#define PREF_UI_WINDOW_HEIGHT  GNOME_VOLUME_CONTROL_KEY ("ui/window_height")

G_END_DECLS

#endif /* __GVC_KEYS_H__ */

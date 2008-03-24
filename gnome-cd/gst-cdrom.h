/*
 *  Authors: Iain Holmes <iain@ximian.com>
 *
 *  Copyright 2002 Iain Holmes
 *  Copyright 2005 Tim-Philipp Müller <tim centricular net>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 *
 */

#ifndef __GST_CDROM_H__
#define __GST_CDROM_H__

#include <glib/gerror.h>
#include <glib-object.h>

#include "cdrom.h"

G_BEGIN_DECLS

#define GST_TYPE_CDROM            (gst_cdrom_get_type ())
#define GST_CDROM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_TYPE_CDROM, GstCDRom))
#define GST_CDROM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GST_TYPE_CDROM, GstCDRomClass))
#define GST_IS_CDROM(obj)         (GTK_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_TYPE_CDROM))
#define GST_IS_CDROM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_TYPE_CDROM))
#define GST_CDROM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_CDROM, GstCDRomClass))

typedef struct _GstCDRom GstCDRom;
typedef struct _GstCDRomClass GstCDRomClass;
typedef struct _GstCDRomPrivate GstCDRomPrivate;

struct _GstCDRom {
  GnomeCDRom cdrom;

  GstCDRomPrivate *priv;
};

struct _GstCDRomClass {
  GnomeCDRomClass klass;
};

GType   gst_cdrom_get_type  (void);

G_END_DECLS

#endif /* __GST_CDROM_H__ */

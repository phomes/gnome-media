/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 *  Authors: Iain Holmes <iain@ximian.com>
 *
 *  Copyright 2001 Iain Holmes
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

#ifndef __BSD_CDROM_H__
#define __BSD_CDROM_H__

#include <glib/gerror.h>
#include <glib-object.h>

#include "cdrom.h"
G_BEGIN_DECLS

#define BSD_CDROM_TYPE (bsd_cdrom_get_type ())
#define BSD_CDROM(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), BSD_CDROM_TYPE, BSDCDRom))
#define BSD_CDROM_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), BSD_CDROM_TYPE, BSDCDRomClass))
#define IS_BSD_CDROM(obj) (GTK_TYPE_CHECK_INSTANCE_TYPE ((obj), BSD_CDROM_TYPE))
#define IS_BSD_CDROM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BSD_CDROM_TYPE))
#define BSD_CDROM_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), BSD_CDROM_TYPE, BSDCDRomClass))

typedef struct _BSDCDRom BSDCDRom;
typedef struct _BSDCDRomPrivate BSDCDRomPrivate;
typedef struct _BSDCDRomClass BSDCDRomClass;

struct _BSDCDRom {
	GnomeCDRom cdrom;

	BSDCDRomPrivate *priv;
};

struct _BSDCDRomClass {
	GnomeCDRomClass klass;
};

GType bsd_cdrom_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif

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

#ifndef __SOLARIS_CDROM_H__
#define __SOLARIS_CDROM_H__

#include <glib/gerror.h>
#include <glib-object.h>

#include "cdrom.h"
G_BEGIN_DECLS

#define SOLARIS_CDROM_TYPE (solaris_cdrom_get_type ())
#define SOLARIS_CDROM(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), SOLARIS_CDROM_TYPE, SolarisCDRom))
#define SOLARIS_CDROM_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), SOLARIS_CDROM_TYPE, SolarisCDRomClass))
#define IS_SOLARIS_CDROM(obj) (GTK_TYPE_CHECK_INSTANCE_TYPE ((obj), SOLARIS_CDROM_TYPE))
#define IS_SOLARIS_CDROM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SOLARIS_CDROM_TYPE))
#define SOLARIS_CDROM_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), SOLARIS_CDROM_TYPE, SolarisCDRomClass))

#define CD_FRAMES 75

typedef struct _SolarisCDRom SolarisCDRom;
typedef struct _SolarisCDRomPrivate SolarisCDRomPrivate;
typedef struct _SolarisCDRomClass SolarisCDRomClass;

struct _SolarisCDRom {
	GnomeCDRom cdrom;

	SolarisCDRomPrivate *priv;
};

struct _SolarisCDRomClass {
	GnomeCDRomClass klass;
};

GType solaris_cdrom_get_type (void) G_GNUC_CONST;

G_END_DECLS

#endif

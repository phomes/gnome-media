/*
 * gst-cdpranoia-cdrom.h
 *
 * Copyright (C) 2001 Iain Holmes
 * Copyright (C) 2003 Jonathan Nall
 * Authors: Iain Holmes  <iain@ximian.com>
 *          Jonathan Nall  <nall@themountaingoats.com>
 */

#ifndef __GST_CDPARANOIA_CDROM_H__
#define __GST_CDPARANOIA_CDROM_H__

#include <glib/gerror.h>
#include <glib-object.h>

#include "cdrom.h"
G_BEGIN_DECLS
#define GST_CDPARANOIA_CDROM_TYPE (gst_cdparanoia_cdrom_get_type ())
#define GST_CDPARANOIA_CDROM(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GST_CDPARANOIA_CDROM_TYPE, GstCdparanoiaCDRom))
#define GST_CDPARANOIA_CDROM_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GST_CDPARANOIA_CDROM_TYPE, GstCdparanoiaCDRomClass))
#define IS_GST_CDPARANOIA_CDROM(obj) (GTK_TYPE_CHECK_INSTANCE_TYPE ((obj), GST_CDPARANOIA_CDROM_TYPE))
#define IS_GST_CDPARANOIA_CDROM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GST_CDPARANOIA_CDROM_TYPE))
#define GST_CDPARANOIA_CDROM_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_CDPARANOIA_CDROM_TYPE, GstCdparanoiaCDRomClass))
typedef struct _GstCdparanoiaCDRom GstCdparanoiaCDRom;
typedef struct _GstCdparanoiaCDRomPrivate GstCdparanoiaCDRomPrivate;
typedef struct _GstCdparanoiaCDRomClass GstCdparanoiaCDRomClass;

struct _GstCdparanoiaCDRom {
	GnomeCDRom cdrom;

	GstCdparanoiaCDRomPrivate *priv;
};

struct _GstCdparanoiaCDRomClass {
	GnomeCDRomClass klass;
};

GType
gst_cdparanoia_cdrom_get_type (void)
    G_GNUC_CONST;

G_END_DECLS
#endif

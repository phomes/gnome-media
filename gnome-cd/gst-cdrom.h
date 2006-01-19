/*
 * gst-cdrom.c: Gstreamer Audio CD controlling functions.
 *
 * Copyright (C) 2005 Tim-Philipp Müller <tim centricular net>
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

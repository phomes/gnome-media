#ifndef OSS_CHANNEL_H
#define OSS_CHANNEL_H

#include <glib/gerror.h>
#include <glib-object.h>

#include "gnome-channel.h"
#include "oss-mixer.h"

G_BEGIN_DECLS

#define OSS_CHANNEL_TYPE (oss_channel_get_type ())
#define OSS_CHANNEL(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), OSS_CHANNEL_TYPE, OssChannel))
#define OSS_CHANNEL_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), OSS_CHANNEL_TYPE, OssChannelClass))
#define IS_OSS_CHANNEL(obj) (GTK_TYPE_CHECK_INSTANCE_TYPE ((obj), OSS_CHANNEL_TYPE))
#define IS_OSS_CHANNEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), OSS_CHANNEL_TYPE))
#define OSS_CHANNEL_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), OSS_CHANNEL_TYPE, OssChannelClass))

typedef struct _OssChannel OssChannel;
typedef struct _OssChannelClass OssChannelClass;
typedef struct _OssChannelPrivate OssChannelPrivate;

struct _OssChannel
{
  GnomeChannel parent_instance;
  OssChannelPrivate *private;
};

struct _OssChannelClass
{
  GnomeChannelClass parent_class;
};

GType oss_channel_get_type          (void);

GObject *oss_channel_new (int device_fd, guint channel_num, gboolean stereo, GError **error);

G_END_DECLS

#endif /* GNOME_CHANNEL_H */

#ifndef OSS_MIXER_H
#define OSS_MIXER_H

#include <glib/gerror.h>
#include <glib-object.h>

#include "gnome-mixer.h"

G_BEGIN_DECLS

#define OSS_MIXER_TYPE (oss_mixer_get_type ())
#define OSS_MIXER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), OSS_MIXER_TYPE, OssMixer))
#define OSS_MIXER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), OSS_MIXER_TYPE, OssMixerClass))
#define IS_OSS_MIXER(obj) (GTK_TYPE_CHECK_INSTANCE_TYPE ((obj), OSS_MIXER_TYPE))
#define IS_OSS_MIXER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), OSS_MIXER_TYPE))
#define OSS_MIXER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), OSS_MIXER_TYPE, OssMixerClass))

typedef struct _OssMixer OssMixer;
typedef struct _OssMixerClass OssMixerClass;
typedef struct _OssMixerPrivate OssMixerPrivate;

struct _OssMixer
{
  GnomeMixer parent_instance;
  OssMixerPrivate *private;
};

struct _OssMixerClass
{
  GnomeMixerClass parent_class;
};

GType oss_mixer_get_type          (void);

GObject  *oss_mixer_new            (guint device_number);


G_END_DECLS

#endif /* GNOME_MIXER_H */

#ifndef ALSA_MIXER_H
#define ALSA_MIXER_H

#include <glib/gerror.h>
#include <glib-object.h>

#include "gnome-mixer.h"

G_BEGIN_DECLS

#define ALSA_MIXER_TYPE (alsa_mixer_get_type ())
#define ALSA_MIXER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), ALSA_MIXER_TYPE, AlsaMixer))
#define ALSA_MIXER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), ALSA_MIXER_TYPE, AlsaMixerClass))
#define IS_ALSA_MIXER(obj) (GTK_TYPE_CHECK_INSTANCE_TYPE ((obj), ALSA_MIXER_TYPE))
#define IS_ALSA_MIXER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ALSA_MIXER_TYPE))
#define ALSA_MIXER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), ALSA_MIXER_TYPE, AlsaMixerClass))

typedef struct _AlsaMixer AlsaMixer;
typedef struct _AlsaMixerClass AlsaMixerClass;
typedef struct _AlsaMixerPrivate AlsaMixerPrivate;

struct _AlsaMixer
{
  GnomeMixer parent_instance;
  AlsaMixerPrivate *private;
};

struct _AlsaMixerClass
{
  GnomeMixerClass parent_class;
};

GType alsa_mixer_get_type          (void);

GObject  *alsa_mixer_new            (guint device_number, GError **error);


G_END_DECLS

#endif /* GNOME_MIXER_H */

#ifndef ALSA_CHANNEL_H
#define ALSA_CHANNEL_H

#include <glib/gerror.h>
#include <glib-object.h>

#include "gnome-channel.h"
#include "alsa-mixer.h"

#include <alsa/asoundlib.h>

G_BEGIN_DECLS

#define ALSA_CHANNEL_TYPE (alsa_channel_get_type ())
#define ALSA_CHANNEL(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), ALSA_CHANNEL_TYPE, AlsaChannel))
#define ALSA_CHANNEL_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), ALSA_CHANNEL_TYPE, AlsaChannelClass))
#define IS_ALSA_CHANNEL(obj) (GTK_TYPE_CHECK_INSTANCE_TYPE ((obj), ALSA_CHANNEL_TYPE))
#define IS_ALSA_CHANNEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ALSA_CHANNEL_TYPE))
#define ALSA_CHANNEL_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), ALSA_CHANNEL_TYPE, AlsaChannelClass))

typedef struct _AlsaChannel AlsaChannel;
typedef struct _AlsaChannelClass AlsaChannelClass;
typedef struct _AlsaChannelPrivate AlsaChannelPrivate;

struct _AlsaChannel
{
  GnomeChannel parent_instance;
  AlsaChannelPrivate *private;
};

struct _AlsaChannelClass
{
  GnomeChannelClass parent_class;
};

GType alsa_channel_get_type          (void);

GObject *alsa_channel_new (snd_mixer_t *mixer_handle, snd_mixer_selem_id_t *sid);

G_END_DECLS

#endif /* GNOME_CHANNEL_H */

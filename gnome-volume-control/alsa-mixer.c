#include "alsa-mixer.h"
#include "alsa-channel.h"

#include <config.h>

#include <alsa/asoundlib.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>

static void build_channel_list (AlsaMixer *mixer);
static gboolean open_device (AlsaMixer *mixer, int device_number, GError **error);

static GnomeMixerClass *parent_class = NULL;

struct _AlsaMixerPrivate {
  GList *channels;

  const char *name;

  snd_mixer_t *mixer_handle;

  int fd;
};

static void
build_channel_list (AlsaMixer *mixer)
{
  int i;
  GObject *channel;

  //for (i=0; i<SOUND_MIXER_NRDEVICES; i++) {
  //  if ((mixer->private->devmask | mixer->private->recmask) & (1<<i)) {
      //channel = alsa_channel_new (mixer->private->fd, /* device file descriptor */
      //				 i, /* channel number */
      //				 mixer->private->stereodevs & (1<<i) /* stereo channel? */
      //				 );
      //mixer->private->channels = g_list_append (mixer->private->channels, channel);

  // }
  //}
}



static gboolean
open_device (AlsaMixer *mixer, int device_number, GError **error)
{
	AlsaMixerPrivate *new_device = mixer->private;
	int err;
	snd_ctl_t *handle;
	snd_mixer_t *mhandle;
	char *card = "default";

	snd_mixer_selem_id_t *sid;
	snd_mixer_elem_t *elem;
 	snd_ctl_card_info_t *info;

	GnomeChannel *channel;

	snd_ctl_card_info_alloca(&info);
	snd_mixer_selem_id_alloca(&sid);

	if ((err = snd_ctl_open(&handle, card, 0)) < 0) {
		g_warning ("Control device %i open error: %s", card, snd_strerror(err));
		return err;
	}
	
	if ((err = snd_ctl_card_info(handle, info)) < 0) {
		g_warning ("Control device %i hw info error: %s", card, snd_strerror(err));
		return err;
	}

	new_device->name = snd_ctl_card_info_get_name(info);

	snd_ctl_close(handle);

	if ((err = snd_mixer_open(&mhandle, 0)) < 0) {
		g_warning ("Mixer %s open error: %s", card, snd_strerror(err));
		return err;
	}
	if ((err = snd_mixer_attach(mhandle, card)) < 0) {
		g_warning ("Mixer attach %s error: %s", card, snd_strerror(err));
		snd_mixer_close(mhandle);
		return err;
	}
	if ((err = snd_mixer_selem_register(mhandle, NULL, NULL)) < 0) {
		g_warning ("Mixer register error: %s", snd_strerror(err));
		snd_mixer_close(mhandle);
		return err;
	}
	err = snd_mixer_load(mhandle);
	if (err < 0) {
		g_warning ("Mixer load error: %s", card, snd_strerror(err));
		snd_mixer_close(mhandle);
		return err;
	}
	for (elem = snd_mixer_first_elem(mhandle); elem; elem = snd_mixer_elem_next(elem)) {
		snd_mixer_selem_get_id(elem, sid);
		if (!snd_mixer_selem_is_active(elem))
			continue;

		channel = GNOME_CHANNEL (alsa_channel_new (mhandle, sid));
		mixer->private->channels = g_list_append (mixer->private->channels, channel);
	}

	return TRUE;
}

static GList *
alsa_mixer_get_channels (GnomeMixer *mixer, GError **error)
{
  AlsaMixer *alsa_mixer = ALSA_MIXER (mixer);
  return alsa_mixer->private->channels;
}

static void
finalize (GObject *object)
{
	AlsaMixer *mixer;

	mixer = ALSA_MIXER (object);

	snd_mixer_close(mixer->private->mixer_handle);

	if (mixer->private == NULL) {
	  return;
	}

	g_free (mixer->private);
	mixer->private = NULL;

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
class_init (AlsaMixerClass *klass)

{
  GObjectClass *object_class;
  GnomeMixerClass *mixer_class;

  object_class = G_OBJECT_CLASS (klass);
  mixer_class = GNOME_MIXER_CLASS (klass);

  object_class->finalize = finalize;

  mixer_class->get_channels = alsa_mixer_get_channels;

  parent_class = g_type_class_peek_parent (klass);
}

static void
init (AlsaMixer *mixer)
{
  mixer->private = g_new (AlsaMixerPrivate, 1);

  mixer->private->channels = NULL;
}

/* API */
GType
alsa_mixer_get_type (void)
{
	static GType type = 0;

	if (type == 0) {
		GTypeInfo info = {
			sizeof (AlsaMixerClass),
			NULL, NULL, (GClassInitFunc) class_init, NULL, NULL,
			sizeof (AlsaMixer), 0, (GInstanceInitFunc) init,
		};

		type = g_type_register_static (GNOME_MIXER_TYPE, "AlsaMixer", &info, 0);
	}

	return type;
}

GObject *
alsa_mixer_new (guint device_number, GError **error)
{
  AlsaMixer *mixer;

  mixer = g_object_new (alsa_mixer_get_type (), NULL);
  
  open_device (mixer, device_number, error);

  return G_OBJECT (mixer);
}

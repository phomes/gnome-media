#include "alsa-channel.h"

#include <config.h>

#include <alsa/asoundlib.h>

#include <gnome.h>
#include <libgnomeui/gnome-window-icon.h>

static GnomeChannelClass *parent_class = NULL;

struct _AlsaChannelPrivate {
  char *name;
  char *pixmap;

  long min_volume;
  long max_volume;

  long volume;

  snd_mixer_t *mixer_handle;
  snd_mixer_selem_id_t *selem_id;
  snd_mixer_elem_t *elem;

  int passive; /* avoid recursive calls to event handler */
};

#ifndef GNOME_STOCK_PIXMAP_BLANK
#define GNOME_STOCK_PIXMAP_BLANK NULL
#endif

struct pixmap {
	char *name;
	const char *pixmap;
};

struct pixmap device_pixmap[] = {
	"Input Gain", GNOME_STOCK_PIXMAP_BLANK,
	"PC Speaker", GNOME_STOCK_VOLUME,
	"MIC", GNOME_STOCK_PIXMAP_BLANK,
	"Line", GNOME_STOCK_LINE_IN,
	"CD", GTK_STOCK_CDROM,
	"Synth", GNOME_STOCK_PIXMAP_BLANK,
	"PCM", GNOME_STOCK_PIXMAP_BLANK,
	"Output Gain", GNOME_STOCK_PIXMAP_BLANK,
	"Treble", GNOME_STOCK_PIXMAP_BLANK,
	"Bass", GNOME_STOCK_PIXMAP_BLANK,
	"Master", GNOME_STOCK_VOLUME,
	"default", GNOME_STOCK_PIXMAP_BLANK,
};

static long
volume_float_to_alsa (float volume, AlsaChannel *channel)
{
  long max = channel->private->max_volume;
  long min = channel->private->min_volume;
  int range = max - min;
  float tmp;

  tmp = ((range) * volume) + min;
  return tmp;
}

static float
volume_alsa_to_float (long volume, AlsaChannel *channel)
{
  long max = channel->private->max_volume;
  long min = channel->private->min_volume;
  int range = max - min;
  float tmp;
  
  if (range == 0)
    return 0.0f;
  volume -= min;
  tmp = (double)volume/(double)range;
  return tmp;
}


static void
alsa_read_in_volume (AlsaChannel *channel)
{
  gboolean pmono;

  if (snd_mixer_selem_has_common_volume(channel->private->elem)
      || snd_mixer_selem_has_playback_volume (channel->private->elem)) {
    snd_mixer_selem_get_playback_volume(channel->private->elem, SND_MIXER_SCHN_MONO, 
					&channel->private->volume);
  }
  //printf ("read volume to be %3.2f\n", volume_alsa_to_float (channel->private->volume, channel));
}

static gfloat
alsa_channel_get_volume (GnomeChannel *channel, GError **error)
{
  AlsaChannel *alsa_channel = ALSA_CHANNEL (channel);
  return volume_alsa_to_float (alsa_channel->private->volume, alsa_channel);
}

static void
alsa_channel_set_volume (GnomeChannel *gnome_channel, gfloat float_volume, GError **error)
{
  AlsaChannel *channel = ALSA_CHANNEL (gnome_channel);

  snd_mixer_selem_set_playback_volume_all (channel->private->elem, volume_float_to_alsa (float_volume, channel));

  alsa_read_in_volume (channel);

  gnome_channel_volume_changed (gnome_channel, float_volume);
}

static char *
alsa_channel_get_name (GnomeChannel *channel, GError **error)
{
  AlsaChannel *alsa_channel = ALSA_CHANNEL (channel);
  return g_strdup (alsa_channel->private->name);
}

static char *
alsa_channel_get_pixmap (GnomeChannel *channel, GError **error)
{
  AlsaChannel *alsa_channel = ALSA_CHANNEL (channel);
  return g_strdup (alsa_channel->private->pixmap);
}

static void
finalize (GObject *object)
{
	AlsaChannel *channel;

	channel = ALSA_CHANNEL (object);
	if (channel->private == NULL) {
	  return;
	}

	g_free (channel->private);
	channel->private = NULL;

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
class_init (AlsaChannelClass *klass)
{
  GObjectClass *object_class;
  GnomeChannelClass *channel_class;

  object_class = G_OBJECT_CLASS (klass);
  channel_class = GNOME_CHANNEL_CLASS (klass);

  object_class->finalize = finalize;

  channel_class->get_volume = alsa_channel_get_volume;
  channel_class->set_volume = alsa_channel_set_volume;
  channel_class->get_name   = alsa_channel_get_name;
  channel_class->get_pixmap = alsa_channel_get_pixmap;

  parent_class = g_type_class_peek_parent (klass);
}

static void
init (AlsaChannel *channel)
{
  channel->private = g_new (AlsaChannelPrivate, 1);
}

/* API */
GType
alsa_channel_get_type (void)
{
	static GType type = 0;

	if (type == 0) {
		GTypeInfo info = {
			sizeof (AlsaChannelClass),
			NULL, NULL, (GClassInitFunc) class_init, NULL, NULL,
			sizeof (AlsaChannel), 0, (GInstanceInitFunc) init,
		};

		type = g_type_register_static (GNOME_CHANNEL_TYPE, "AlsaChannel", &info, 0);
	}

	return type;
}

static gboolean
check_volume_levels (gpointer data)
{
  AlsaChannel *channel = ALSA_CHANNEL (data);

  gnome_channel_volume_changed (GNOME_CHANNEL (channel), 
				1.0f);//volume_alsa_to_float (channel->private->volume_left));

  return TRUE;
}

GObject *
alsa_channel_new (snd_mixer_t *mixer_handle, snd_mixer_selem_id_t *sid)
{
  AlsaChannel *channel;
  int j;

  channel = g_object_new (alsa_channel_get_type (), NULL);

  channel->private->name          =  g_strdup (snd_mixer_selem_id_get_name (sid));
  for (j=0; strcmp(device_pixmap[j].name, "default")!=0 &&
	 strcmp(device_pixmap[j].name, channel->private->name) !=0; j++);
  channel->private->pixmap        = g_strdup (device_pixmap[j].pixmap);
  channel->private->mixer_handle  =  mixer_handle;
  channel->private->selem_id      =  sid;
  channel->private->elem          = snd_mixer_find_selem(mixer_handle, sid);

  //printf("Simple mixer control '%s',%i\n", snd_mixer_selem_id_get_name(sid), snd_mixer_selem_id_get_index(sid));

  if (snd_mixer_selem_has_common_volume(channel->private->elem)
      || snd_mixer_selem_has_playback_volume (channel->private->elem)) {
    snd_mixer_selem_get_playback_volume_range(channel->private->elem, 
					      &(channel->private->min_volume), 
					      &(channel->private->max_volume));
  } else {
    channel->private->min_volume = 0;
    channel->private->max_volume = 100;
  }

  alsa_read_in_volume (channel);
  
  //g_timeout_add (500, &check_volume_levels, channel);

  //show_selem (mixer_handle, sid, "  ");

  return G_OBJECT (channel);
}

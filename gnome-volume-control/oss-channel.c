#include "oss-channel.h"

#include <config.h>

#ifdef HAVE_LINUX_SOUNDCARD_H 
#include <linux/soundcard.h>
#else 
#include <machine/soundcard.h>
#endif

#include <gnome.h>
#include <libgnomeui/gnome-window-icon.h>

static GnomeChannelClass *parent_class = NULL;

struct _OssChannelPrivate {
  guint volume_left;
  guint volume_right;

  char *title;
  char *user_title;
  char *pixmap;

  gboolean stereo;
  guint channel_number;
  int device_fd;

  int passive; /* avoid recursive calls to event handler */
};

#define NICE_SOUND_DEVICE_LABELS	{"Volume", "Bass", "Treble", "Synth", "Wave", "PC Speaker ", "Line", \
				         "Microphone", "CD", "Mix  ", "Pcm2 ", "Recording", "IGain", "OGain", \
				         "Line 1", "Line 2", "Line 3", "Digital 1", "Digital 2", "Digital3 ", \
				         "PhoneIn", "PhoneOut", "Video", "Radio", "Monitor"}
static const char *device_labels[] = NICE_SOUND_DEVICE_LABELS;
static const char *device_names[]  = SOUND_DEVICE_NAMES;
#ifndef GNOME_STOCK_PIXMAP_BLANK
#define GNOME_STOCK_PIXMAP_BLANK NULL
#endif
static const char *device_pixmap[] = {
	GNOME_STOCK_PIXMAP_VOLUME,               /* Master Volume */
	GNOME_STOCK_PIXMAP_BLANK,                /* Bass */
	GNOME_STOCK_PIXMAP_BLANK,                /* Treble */
	GNOME_STOCK_PIXMAP_BLANK,                /* Synth */
	GNOME_STOCK_PIXMAP_BLANK,                /* PCM */
	GNOME_STOCK_PIXMAP_VOLUME,               /* Speaker */
	GNOME_STOCK_PIXMAP_LINE_IN,              /* Line In */
	GNOME_STOCK_PIXMAP_MIC,                  /* Microphone */
	GNOME_STOCK_PIXMAP_CDROM,                /* CD-Rom */
	GNOME_STOCK_PIXMAP_BLANK,                /* Recording monitor ? */
	GNOME_STOCK_PIXMAP_BLANK,                /* ALT PCM */
	GNOME_STOCK_PIXMAP_BLANK,                /* Rec Level? */
	GNOME_STOCK_PIXMAP_BLANK,                /* In Gain */
	GNOME_STOCK_PIXMAP_BLANK,                /* Out Gain */
	GNOME_STOCK_PIXMAP_LINE_IN,              /* Aux 1 */
	GNOME_STOCK_PIXMAP_LINE_IN,              /* Aux 2 */
	GNOME_STOCK_PIXMAP_LINE_IN,              /* Line */
	GNOME_STOCK_PIXMAP_BLANK,                /* Digital 1 ? */
	GNOME_STOCK_PIXMAP_BLANK,                /* Digital 2 ? */
	GNOME_STOCK_PIXMAP_BLANK,                /* Digital 3 ? */
	GNOME_STOCK_PIXMAP_BLANK,                /* Phone in */
	GNOME_STOCK_PIXMAP_BLANK,                /* Phone Out */
	GNOME_STOCK_PIXMAP_BLANK,                /* Video */
	GNOME_STOCK_PIXMAP_BLANK,                /* Radio */
	GNOME_STOCK_PIXMAP_BLANK,                /* Monitor (usually mic) vol */
	GNOME_STOCK_PIXMAP_BLANK,                /* 3d Depth/space param */
	GNOME_STOCK_PIXMAP_BLANK,                /* 3d center param */
	GNOME_STOCK_PIXMAP_BLANK                 /* Midi */
};

static void
oss_read_in_volume (OssChannel *channel)
{
  unsigned long vol; /* l: vol&0xff, r:(vol&0xff00)>>8 */
  int res;

  res = ioctl (channel->private->device_fd, MIXER_READ(channel->private->channel_number), &vol);
  
  channel->private->volume_left = vol & 0xff;

  if (channel->private->stereo) {
    channel->private->volume_right = (vol & 0xff00) >> 8;
  } else {
    channel->private->volume_right = channel->private->volume_left;
  }
}

static guint
volume_float_to_oss (float volume)
{
  return (guint) (volume * 101.0f);
}

static float
volume_oss_to_float (guint volume)
{
  return ((float)volume) / 101.0f;
}

static gfloat
oss_channel_get_volume (GnomeChannel *channel, GError **error)
{
  OssChannel *oss_channel = OSS_CHANNEL (channel);
  return volume_oss_to_float (oss_channel->private->volume_left);
}

static void
oss_channel_set_volume (GnomeChannel *gnome_channel, gfloat float_volume, GError **error)
{
  OssChannel *channel = OSS_CHANNEL (gnome_channel);
  unsigned long vol;
  guint volume;

  volume = volume_float_to_oss (float_volume);

  if (channel->private->stereo) {
    channel->private->volume_right = volume;
    channel->private->volume_left  = volume;
  } else {
    channel->private->volume_left = volume;
  }
  
  vol =  channel->private->volume_left;
  vol |= channel->private->volume_right << 8;

  ioctl(channel->private->device_fd, MIXER_WRITE(channel->private->channel_number), &vol);
  oss_read_in_volume (channel);

  gnome_channel_volume_changed (gnome_channel, float_volume);
}

static char *
oss_channel_get_name (GnomeChannel *channel, GError **error)
{
  OssChannel *oss_channel = OSS_CHANNEL (channel);
  return g_strdup (oss_channel->private->title);
}

static char *
oss_channel_get_pixmap (GnomeChannel *channel, GError **error)
{
  OssChannel *oss_channel = OSS_CHANNEL (channel);
  return g_strdup (oss_channel->private->pixmap);
}

static void
finalize (GObject *object)
{
	OssChannel *channel;

	channel = OSS_CHANNEL (object);
	if (channel->private == NULL) {
	  return;
	}

	g_free (channel->private);
	channel->private = NULL;

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
class_init (OssChannelClass *klass)
{
  GObjectClass *object_class;
  GnomeChannelClass *channel_class;

  object_class = G_OBJECT_CLASS (klass);
  channel_class = GNOME_CHANNEL_CLASS (klass);

  object_class->finalize = finalize;

  channel_class->get_volume = oss_channel_get_volume;
  channel_class->set_volume = oss_channel_set_volume;
  channel_class->get_name   = oss_channel_get_name;
  channel_class->get_pixmap = oss_channel_get_pixmap;

  parent_class = g_type_class_peek_parent (klass);
}

static void
init (OssChannel *channel)
{
  channel->private = g_new (OssChannelPrivate, 1);
}

/* API */
GType
oss_channel_get_type (void)
{
	static GType type = 0;

	if (type == 0) {
		GTypeInfo info = {
			sizeof (OssChannelClass),
			NULL, NULL, (GClassInitFunc) class_init, NULL, NULL,
			sizeof (OssChannel), 0, (GInstanceInitFunc) init,
		};

		type = g_type_register_static (GNOME_CHANNEL_TYPE, "OssChannel", &info, 0);
	}

	return type;
}

static gboolean
check_volume_levels (gpointer data)
{
  guint left, right;
  OssChannel *channel = OSS_CHANNEL (data);

  left = channel->private->volume_left;
  right = channel->private->volume_right;

  oss_read_in_volume (channel);

  if ((left != channel->private->volume_left)
      || (right != channel->private->volume_right)) {
    gnome_channel_volume_changed (GNOME_CHANNEL (channel), 
				  volume_oss_to_float (channel->private->volume_left));
  }

  return TRUE;
}

GObject *
oss_channel_new (int device_fd, guint channel_num, gboolean stereo_channel, GError **error)
{
  OssChannel *channel;

  channel = g_object_new (oss_channel_get_type (), NULL);

  channel->private->pixmap     = g_strdup (device_pixmap[channel_num]);
  channel->private->title      = g_strdup (_(device_labels[channel_num]));
  channel->private->user_title = g_strdup (_(device_labels[channel_num]));
  channel->private->passive    = 0;
  channel->private->channel_number = channel_num;
  channel->private->device_fd = device_fd;

  channel->private->stereo     = stereo_channel;

  oss_read_in_volume (channel);
  
  g_timeout_add (500, &check_volume_levels, channel);

  return G_OBJECT (channel);
}

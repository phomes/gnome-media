#include "oss-mixer.h"
#include "oss-channel.h"

#include <config.h>

#ifdef HAVE_LINUX_SOUNDCARD_H 
#include <linux/soundcard.h>
#else 
#include <machine/soundcard.h>
#endif

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

#include <errno.h>

static void build_channel_list (OssMixer *mixer, GError **error);
static gboolean open_device (OssMixer *mixer, int device_number, GError **error);

static GnomeMixerClass *parent_class = NULL;

struct _OssMixerPrivate {
  GList *channels;

  int fd;
  mixer_info info;
  int recsrc;	/* current recording-source(s) */
  int devmask;	/* devices supported by this driver */
  int recmask;	/* devices, that can be a recording-source */
  int stereodevs;	/* devices with stereo abilities */
  int caps;	/* features supported by the mixer */
  int volume_left[32], volume_right[32]; /* volume, mono only left */
  
  int mute_bitmask; /* which channels are muted */
  int lock_bitmask; /* which channels have the L/R sliders linked together */
  int enabled_bitmask; /* which channels should be visible in the GUI ? */
};

static void
build_channel_list (OssMixer *mixer, GError **error)
{
  int i;
  GObject *channel;

  for (i=0; i<SOUND_MIXER_NRDEVICES; i++) {
    if ((mixer->private->devmask | mixer->private->recmask) & (1<<i)) {
      channel = oss_channel_new (mixer->private->fd, /* device file descriptor */
				 i, /* channel number */
				 mixer->private->stereodevs & (1<<i), /* stereo channel? */
				 error);
      if (error != NULL) {
	mixer->private->channels = g_list_append (mixer->private->channels, channel);
      }
    }
  }
}

static void
generic_error (GError **error, char *device_name)
{
  if (error) {
    if (errno == EPERM || errno == EACCES || errno == ENXIO) {
      *error = g_error_new (GNOME_MIXER_ERROR,
			    GNOME_MIXER_ERROR_PERMISSION,
			    "The mixer device %s couldn't be used because"
			    " of insufficient permissions.",
			    device_name
			    );
    } else {
      *error = g_error_new (GNOME_MIXER_ERROR,
			    GNOME_MIXER_ERROR_UNKNOWN,
			    "The mixer device %s couldn't be used.",
			    device_name
			    );
    }
  }
}

static gboolean
open_device (OssMixer *mixer, int device_number, GError **error)
{
	OssMixerPrivate *new_device = mixer->private;
	char device_name[255];
	int res, ver, cnt;

	/*
	 * open the mixer-device
	 */
	if (device_number==0) {
		sprintf(device_name, "/dev/mixer");
	} else {
		sprintf(device_name, "/dev/mixer%i", device_number);
	}

	
	if (!g_file_test (device_name, G_FILE_TEST_EXISTS)) {
	  if (error) {
	    *error = g_error_new (GNOME_MIXER_ERROR,
				  GNOME_MIXER_ERROR_NO_SUCH_DEVICE,
				  "The mixer device %s couldn't be found.",
				  device_name
				  );
	  }
	  return FALSE;
	}

	new_device->fd = open(device_name, O_RDWR, 0);
	if (new_device->fd<0) {
	  printf ("errno is %d\n", errno);
	  generic_error (error, device_name);
	  return FALSE;
	}

	/*
	 * mixer-name
	 */
	res=ioctl(new_device->fd, SOUND_MIXER_INFO, &new_device->info);
	if (res!=0) {
	  generic_error (error, device_name);
	  return FALSE;
	}
	if(!isalpha(new_device->info.name[0]))
		g_snprintf(new_device->info.name, 31, "Card %d", device_number+1);
	/* 
	 * several bitmasks describing the mixer
	 */
	res=ioctl(new_device->fd, SOUND_MIXER_READ_DEVMASK, &new_device->devmask);
        res|=ioctl(new_device->fd, SOUND_MIXER_READ_RECMASK, &new_device->recmask);
        res|=ioctl(new_device->fd, SOUND_MIXER_READ_RECSRC, &new_device->recsrc);
        res|=ioctl(new_device->fd, SOUND_MIXER_READ_STEREODEVS, &new_device->stereodevs);
        res|=ioctl(new_device->fd, SOUND_MIXER_READ_CAPS, &new_device->caps);
	if (res!=0) {
	  generic_error (error, device_name);
	  return FALSE;
	}

	/*
	 * get current volume
	 */
	new_device->mute_bitmask=0;				/* not muted */
	new_device->lock_bitmask=new_device->stereodevs;	/* all locked */
	new_device->enabled_bitmask=new_device->devmask;	/* all enabled */
	for (cnt=0; cnt<SOUND_MIXER_NRDEVICES; cnt++) {
		if (new_device->devmask & (1<<cnt)) {
			unsigned long vol; /* l: vol&0xff, r:(vol&0xff00)>>8 */
			res=ioctl(new_device->fd, MIXER_READ(cnt), &vol);
		                                                
			new_device->volume_left[cnt]=vol & 0xff;
			if (new_device->stereodevs & (1<<cnt)) {
				new_device->volume_right[cnt]=(vol&0xff00)>>8;
			} else {
				new_device->volume_right[cnt]=vol&0xff;
			}
		}
	}
	return TRUE;
}

static GList *
oss_mixer_get_channels (GnomeMixer *mixer, GError **error)
{
  OssMixer *oss_mixer = OSS_MIXER (mixer);
  return oss_mixer->private->channels;
}

static void
finalize (GObject *object)
{
	OssMixer *mixer;

	mixer = OSS_MIXER (object);
	if (mixer->private == NULL) {
	  return;
	}

	g_free (mixer->private);
	mixer->private = NULL;

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
class_init (OssMixerClass *klass)

{
  GObjectClass *object_class;
  GnomeMixerClass *mixer_class;

  object_class = G_OBJECT_CLASS (klass);
  mixer_class = GNOME_MIXER_CLASS (klass);

  object_class->finalize = finalize;

  mixer_class->get_channels = oss_mixer_get_channels;

  parent_class = g_type_class_peek_parent (klass);
}

static void
init (OssMixer *mixer)
{
  mixer->private = g_new (OssMixerPrivate, 1);

  mixer->private->channels = NULL;
}

/* API */
GType
oss_mixer_get_type (void)
{
	static GType type = 0;

	if (type == 0) {
		GTypeInfo info = {
			sizeof (OssMixerClass),
			NULL, NULL, (GClassInitFunc) class_init, NULL, NULL,
			sizeof (OssMixer), 0, (GInstanceInitFunc) init,
		};

		type = g_type_register_static (GNOME_MIXER_TYPE, "OssMixer", &info, 0);
	}

	return type;
}

GObject *
oss_mixer_new (guint device_number, GError **error)
{
  OssMixer *mixer;

  mixer = g_object_new (oss_mixer_get_type (), NULL);
  
  if (open_device (mixer, device_number, error)) {
    build_channel_list (mixer, error);
  } else {
    g_object_unref (G_OBJECT (mixer));
    return NULL;
  }

  return G_OBJECT (mixer);
}

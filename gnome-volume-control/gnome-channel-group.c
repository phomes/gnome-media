#include "gnome-channel-group.h"
#include "volume-marshal.h"

#include <config.h>

#ifdef HAVE_LINUX_SOUNDCARD_H 
#include <linux/soundcard.h>
#else 
#include <machine/soundcard.h>
#endif

static GnomeChannelClass *parent_class = NULL;
enum {
  WEIGHT_CHANGED,
  LAST_SIGNAL
};

static gulong gnome_channel_group_signals[LAST_SIGNAL] = { 0, };


typedef struct {
  GnomeChannel *channel;
  GnomeChannelGroup *parent;
  gfloat weight;
  gboolean just_triggered;
} ChannelInfo;

struct _GnomeChannelGroupPrivate {
  GList *channel_weights;
  GnomeChannel *master;

  char *pixmap;
  char *title;

  gfloat volume;
};

static void
group_update_volume (GnomeChannelGroup *channel, GError **error)
{
  GList *node;
  ChannelInfo *channel_info;
  gfloat max_volume = 0.0f;
  gfloat volume;
  gfloat new_weight;
  gboolean weight_changed = FALSE;
  int i;

  for (node = channel->private->channel_weights; node != NULL; node = node->next) {
    channel_info = (ChannelInfo *) node->data;
    
    volume = gnome_channel_get_volume (channel_info->channel, NULL);
    
    if (volume > max_volume) {
      max_volume = volume;
    }
  }

  if (channel->private->volume != max_volume) {
    gnome_channel_volume_changed (GNOME_CHANNEL (channel), max_volume);
  }

  channel->private->volume = max_volume;

  i = 0;
  for (node = channel->private->channel_weights; node != NULL; node = node->next) {
    channel_info = (ChannelInfo *) node->data;

    /* deal with divide by zero */
    if (channel->private->volume == 0.0f) {
      new_weight = 1.0f;
    } else {
      new_weight = gnome_channel_get_volume (channel_info->channel, NULL) / (channel->private->volume);
    }

    if (new_weight != channel_info->weight) {
      //printf ("weight changed for channel %s from %3.2f to %3.2f\n", gnome_channel_get_name (channel_info->channel, error), channel_info->weight, new_weight);
      gnome_channel_group_weight_changed (channel, i, new_weight);
    }
    channel_info->weight = new_weight;
    i++;
  }
}


static void
volume_changed_cb (GnomeChannel *channel, float volume, ChannelInfo *info)
{
  if (info->just_triggered == TRUE) {
    info->just_triggered = FALSE;
  } else {
    group_update_volume (info->parent, NULL);
  }
}


static gfloat
gnome_channel_group_get_volume (GnomeChannel *channel, GError **error)
{
  GnomeChannelGroup *gnome_channel_group = GNOME_CHANNEL_GROUP (channel);
  return gnome_channel_group->private->volume;
}

static void
gnome_channel_group_set_volume (GnomeChannel *gnome_channel, gfloat volume, GError **error)
{
  GList *node;
  GnomeChannelGroup *channel = GNOME_CHANNEL_GROUP (gnome_channel);
  ChannelInfo *info;
  GnomeChannel *subchannel;
  gfloat subchannel_volume;

  for (node = channel->private->channel_weights; node != NULL; node = node->next) {  
    info = (ChannelInfo *) node->data;

    subchannel_volume =  volume * info->weight;
    info->just_triggered = TRUE;
    
    if (!strcmp ("Microphone", gnome_channel_get_name (info->channel, NULL))) {
      printf ("volume changed for mic from:  %3.2f -> %3.2f\n", gnome_channel_get_volume (info->channel, NULL), subchannel_volume);
    }
    gnome_channel_set_volume (info->channel, subchannel_volume, error);
  }

  group_update_volume (channel, error);
}

static char *
gnome_channel_group_get_name (GnomeChannel *channel, GError **error)
{
  GnomeChannelGroup *gnome_channel_group = GNOME_CHANNEL_GROUP (channel);
  return g_strdup (gnome_channel_group->private->title);
}

static char *
gnome_channel_group_get_pixmap (GnomeChannel *channel, GError **error)
{
  GnomeChannelGroup *gnome_channel_group = GNOME_CHANNEL_GROUP (channel);
  return g_strdup (gnome_channel_group->private->pixmap);
}

void
gnome_channel_group_weight_changed (GnomeChannelGroup *group, int channel_num, gfloat weight)
{
    g_signal_emit (G_OBJECT (group), gnome_channel_group_signals[WEIGHT_CHANGED], 0, channel_num, weight);
}

static void
finalize (GObject *object)
{
	GnomeChannelGroup *channel;

	channel = GNOME_CHANNEL_GROUP (object);
	if (channel->private == NULL) {
	  return;
	}

	g_free (channel->private);
	channel->private = NULL;

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
class_init (GnomeChannelGroupClass *klass)
{
  GObjectClass *object_class;
  GnomeChannelClass *channel_class;

  object_class = G_OBJECT_CLASS (klass);
  channel_class = GNOME_CHANNEL_CLASS (klass);

  object_class->finalize = finalize;

  channel_class->get_volume = gnome_channel_group_get_volume;
  channel_class->set_volume = gnome_channel_group_set_volume;
  channel_class->get_name   = gnome_channel_group_get_name;
  channel_class->get_pixmap = gnome_channel_group_get_pixmap;

  gnome_channel_group_signals[WEIGHT_CHANGED] = g_signal_new ("weight_changed",
							      G_TYPE_FROM_CLASS (klass),
							      G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE,
							      G_STRUCT_OFFSET (GnomeChannelGroupClass, weight_changed),
							      NULL, NULL,
							      volume_control_marshal_VOID__INT_FLOAT,
							      G_TYPE_NONE,
							      2, G_TYPE_INT, G_TYPE_FLOAT);
  
  parent_class = g_type_class_peek_parent (klass);
}

static void
init (GnomeChannelGroup *channel)
{
  channel->private = g_new (GnomeChannelGroupPrivate, 1);
}

/* API */
GType
gnome_channel_group_get_type (void)
{
	static GType type = 0;

	if (type == 0) {
		GTypeInfo info = {
			sizeof (GnomeChannelGroupClass),
			NULL, NULL, (GClassInitFunc) class_init, NULL, NULL,
			sizeof (GnomeChannelGroup), 0, (GInstanceInitFunc) init,
		};

		type = g_type_register_static (GNOME_CHANNEL_TYPE, "GnomeChannelGroup", &info, 0);
	}

	return type;
}

GObject *
gnome_channel_group_new (GList *channels, const char *name, const char *pixmap)
{
  GnomeChannelGroup *channel;
  GList *node;
  ChannelInfo *info;

  channel = g_object_new (gnome_channel_group_get_type (), NULL);

  channel->private->pixmap     = g_strdup (pixmap);
  channel->private->title      = g_strdup (name);

  channel->private->channel_weights = NULL;

  for (node = channels; node != NULL; node = node->next) {
    info = g_new (ChannelInfo, 1);
    g_object_ref (GNOME_CHANNEL (node->data));
    info->channel = GNOME_CHANNEL (node->data);
    info->weight = 1.0f;
    info->just_triggered = FALSE;
    info->parent = channel;

    g_signal_connect (G_OBJECT (info->channel),
		      "volume_changed",
		      G_CALLBACK (volume_changed_cb),
		      info);

    channel->private->channel_weights = g_list_append (channel->private->channel_weights,
						       info);

  }

  group_update_volume (channel, NULL);

  return G_OBJECT (channel);
}

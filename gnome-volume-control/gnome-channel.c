#include "gnome-channel.h"

static GObjectClass *parent_class = NULL;

enum {
	VOLUME_CHANGED,
	GET_VOLUME,
	SET_VOLUME,
	LAST_SIGNAL
};

static gulong gnome_channel_signals[LAST_SIGNAL] = { 0, };

struct _GnomeChannelPrivate {
};

static gfloat
channel_get_volume (GnomeChannel *channel, GError **error)
{
}

static void
channel_set_volume (GnomeChannel *channel, gfloat volume, GError **error)
{
}

static char *
channel_get_name (GnomeChannel *channel, GError **error)
{
}

static char *
channel_get_pixmap (GnomeChannel *channel, GError **error)
{
}

gfloat
gnome_channel_get_volume (GnomeChannel *channel, GError **error)
{
  GnomeChannelClass *klass = GNOME_CHANNEL_GET_CLASS (channel);
  return klass->get_volume (channel, error);
}

void gnome_channel_set_volume (GnomeChannel *channel, gfloat volume, GError **error)
{
  GnomeChannelClass *klass = GNOME_CHANNEL_GET_CLASS (channel);
  return klass->set_volume (channel, volume, error);
}

char *
gnome_channel_get_name (GnomeChannel *channel, GError **error)
{
  GnomeChannelClass *klass = GNOME_CHANNEL_GET_CLASS (channel);
  return klass->get_name (channel, error);
}

char *
gnome_channel_get_pixmap (GnomeChannel *channel, GError **error)
{
  GnomeChannelClass *klass = GNOME_CHANNEL_GET_CLASS (channel);
  return klass->get_pixmap (channel, error);
}

void
gnome_channel_volume_changed (GnomeChannel *channel,
			      gfloat new_volume)
{
  g_signal_emit (G_OBJECT (channel), gnome_channel_signals[VOLUME_CHANGED], 0, new_volume);
}

static void
finalize (GObject *object)
{
  GnomeChannel *channel;

  channel = GNOME_CHANNEL (object);

  if (channel->private == NULL) {
    return;
  }
      
  g_free (channel->private);
  channel->private = NULL;

  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
init (GnomeChannel *channel)
{
  channel->private = g_new (GnomeChannelPrivate, 1);
}

static void
class_init (GnomeChannelClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = finalize;

	klass->get_volume = channel_get_volume;
	klass->set_volume = channel_set_volume;
	klass->get_name   = channel_get_name;
	klass->get_pixmap = channel_get_pixmap;

	/* Signals */
	gnome_channel_signals[VOLUME_CHANGED] = g_signal_new ("volume_changed",
							      G_TYPE_FROM_CLASS (klass),
							      G_SIGNAL_RUN_FIRST | G_SIGNAL_NO_RECURSE,
							      G_STRUCT_OFFSET (GnomeChannelClass, volume_changed),
							      NULL, NULL,
							      g_cclosure_marshal_VOID__FLOAT,
							      G_TYPE_NONE,
							      1, G_TYPE_FLOAT);
	parent_class = g_type_class_peek_parent (klass);
}

GType
gnome_channel_get_type (void)
{
        static GType type = 0;

        if (type == 0) {
                GTypeInfo info = {
                        sizeof (GnomeChannelClass),
                        NULL, NULL, (GClassInitFunc) class_init, NULL, NULL,
                        sizeof (GnomeChannel), 0, (GInstanceInitFunc) init,
                };

                type = g_type_register_static (G_TYPE_OBJECT, "GnomeChannel", &info, 0);
        }

        return type;
}

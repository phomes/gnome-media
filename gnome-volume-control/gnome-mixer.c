#include "gnome-mixer.h"

static GObjectClass *parent_class = NULL;

static GList *
mixer_get_channels (GnomeMixer *mixer, GError **error)
{
}

GList *
gnome_mixer_get_channels (GnomeMixer *mixer, GError **error)
{
  GnomeMixerClass *klass = GNOME_MIXER_GET_CLASS (mixer);
  return klass->get_channels (mixer, error);
}


static void
finalize (GObject *object)
{
  G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
init (GnomeMixer *mixer)
{
}

static void
class_init (GnomeMixerClass *klass)
{
	GObjectClass *object_class;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = finalize;

	klass->get_channels = mixer_get_channels;

	parent_class = g_type_class_peek_parent (klass);
}

GQuark
gnome_mixer_error_quark (void)
{
        static GQuark quark = 0;
        if (quark == 0) {
                quark = g_quark_from_static_string ("gnome-mixer-error-quark");
        }

        return quark;
}

GType
gnome_mixer_get_type (void)
{
        static GType type = 0;

        if (type == 0) {
                GTypeInfo info = {
                        sizeof (GnomeMixerClass),
                        NULL, NULL, (GClassInitFunc) class_init, NULL, NULL,
                        sizeof (GnomeMixer), 0, (GInstanceInitFunc) init,
                };

                type = g_type_register_static (G_TYPE_OBJECT, "GnomeMixer", &info, 0);
        }

        return type;
}

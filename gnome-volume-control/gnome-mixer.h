#ifndef GNOME_MIXER_H
#define GNOME_MIXER_H

#include <glib/gerror.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define GNOME_MIXER_TYPE (gnome_mixer_get_type ())
#define GNOME_MIXER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GNOME_MIXER_TYPE, GnomeMixer))
#define GNOME_MIXER_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GNOME_MIXER_TYPE, GnomeMixerClass))
#define IS_GNOME_MIXER(obj) (GTK_TYPE_CHECK_INSTANCE_TYPE ((obj), GNOME_MIXER_TYPE))
#define IS_GNOME_MIXER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GNOME_MIXER_TYPE))
#define GNOME_MIXER_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GNOME_MIXER_TYPE, GnomeMixerClass))

#define GNOME_MIXER_ERROR gnome_mixer_error_quark ()

typedef struct _GnomeMixer GnomeMixer;
typedef struct _GnomeMixerClass GnomeMixerClass;
typedef struct _GnomeMixerPrivate GnomeMixerPrivate;

struct _GnomeMixer
{
  GObject parent_instance;
};

struct _GnomeMixerClass
{
  GObjectClass parent_class;

  GList * (*get_channels) (GnomeMixer *mixer,
			   GError **error);
};

typedef enum {
        GNOME_MIXER_ERROR_NOT_IMPLEMENTED,
	GNOME_MIXER_ERROR_UNKNOWN,
        GNOME_MIXER_ERROR_NO_SUCH_DEVICE,
	GNOME_MIXER_ERROR_PERMISSION
} GnomeMixerError;

GType   gnome_mixer_get_type          (void);
GList  *gnome_mixer_get_channels      (GnomeMixer *mixer, GError **error);
GQuark  gnome_cdrom_error_quark       (void);

G_END_DECLS

#endif /* GNOME_MIXER_H */

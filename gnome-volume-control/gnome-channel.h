#ifndef GNOME_CHANNEL_H
#define GNOME_CHANNEL_H

#include <glib/gerror.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define GNOME_CHANNEL_TYPE (gnome_channel_get_type ())
#define GNOME_CHANNEL(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GNOME_CHANNEL_TYPE, GnomeChannel))
#define GNOME_CHANNEL_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GNOME_CHANNEL_TYPE, GnomeChannelClass))
#define IS_GNOME_CHANNEL(obj) (GTK_TYPE_CHECK_INSTANCE_TYPE ((obj), GNOME_CHANNEL_TYPE))
#define IS_GNOME_CHANNEL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GNOME_CHANNEL_TYPE))
#define GNOME_CHANNEL_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GNOME_CHANNEL_TYPE, GnomeChannelClass))

typedef struct _GnomeChannel GnomeChannel;
typedef struct _GnomeChannelClass GnomeChannelClass;
typedef struct _GnomeChannelPrivate GnomeChannelPrivate;

struct _GnomeChannel
{
  GObject parent_instance;
  GnomeChannelPrivate *private;
};

struct _GnomeChannelClass
{
  GObjectClass parent_class;

  gfloat (*get_volume) (GnomeChannel *channel,
			GError **error);
  void   (*set_volume) (GnomeChannel *channel,
			gfloat volume,
			GError **error);
  char * (*get_name)   (GnomeChannel *channel,
			GError **error);
  char * (*get_pixmap) (GnomeChannel *channel,
			GError **error);


  /* Signals */
  void (*volume_changed) (GnomeChannel *channel,
			  gfloat volume)  ;
};

GType           gnome_channel_get_type          (void);

gfloat   gnome_channel_get_volume      (GnomeChannel *channel, GError **error);
void     gnome_channel_set_volume      (GnomeChannel *channel, gfloat volume, GError **error);
char *   gnome_channel_get_name        (GnomeChannel *channel, GError **error);
char *   gnome_channel_get_pixmap      (GnomeChannel *channel, GError **error);

void     gnome_channel_volume_changed  (GnomeChannel *channel, gfloat volume);

G_END_DECLS

#endif /* GNOME_CHANNEL_H */

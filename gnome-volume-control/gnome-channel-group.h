#ifndef GNOME_CHANNEL_GROUP_H
#define GNOME_CHANNEL_GROUP_H

#include <glib/gerror.h>
#include <glib-object.h>

#include "gnome-channel.h"
#include "oss-mixer.h"

G_BEGIN_DECLS

#define GNOME_CHANNEL_GROUP_TYPE (gnome_channel_group_get_type ())
#define GNOME_CHANNEL_GROUP(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), GNOME_CHANNEL_GROUP_TYPE, GnomeChannelGroup))
#define GNOME_CHANNEL_GROUP_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST ((klass), GNOME_CHANNEL_GROUP_TYPE, GnomeChannelGroupClass))
#define IS_GNOME_CHANNEL_GROUP(obj) (GTK_TYPE_CHECK_INSTANCE_TYPE ((obj), GNOME_CHANNEL_GROUP_TYPE))
#define IS_GNOME_CHANNEL_GROUP_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GNOME_CHANNEL_GROUP_TYPE))
#define GNOME_CHANNEL_GROUP_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GNOME_CHANNEL_GROUP_TYPE, GnomeChannelGroupClass))

typedef struct _GnomeChannelGroup GnomeChannelGroup;
typedef struct _GnomeChannelGroupClass GnomeChannelGroupClass;
typedef struct _GnomeChannelGroupPrivate GnomeChannelGroupPrivate;

struct _GnomeChannelGroup
{
  GnomeChannel parent_instance;
  GnomeChannelGroupPrivate *private;
};

struct _GnomeChannelGroupClass
{
  GnomeChannelClass parent_class;

  /* Signals */
  void (*weight_changed) (GnomeChannelGroup *group,
			  int channel_number, gfloat weight);
};

GType gnome_channel_group_get_type          (void);

void     gnome_channel_group_weight_changed  (GnomeChannelGroup *group, int channel_num, gfloat weight);
GObject *gnome_channel_group_new             (GList *channels, const char *name, const char *pixmap);

G_END_DECLS

#endif /* GNOME_CHANNEL_H */

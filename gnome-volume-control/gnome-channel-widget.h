#ifndef GNOME_CHANNEL_WIDGET_H
#define GNOME_CHANNEL_WIDGET_H

#include "gnome-channel.h"
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define GNOME_TYPE_CHANNEL_WIDGET		 (gnome_channel_widget_get_type ())
#define GNOME_CHANNEL_WIDGET(obj)		 (GTK_CHECK_CAST ((obj), GNOME_TYPE_CHANNEL_WIDGET, GnomeChannelWidget))
#define GNOME_CHANNEL_WIDGET_CLASS(klass)	 (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_CHANNEL_WIDGET, GnomeChannelWidgetClass))
#define GNOME_IS_CHANNEL_WIDGET(obj)	 (GTK_CHECK_TYPE ((obj), GNOME_TYPE_CHANNEL_WIDGET))
#define GNOME_IS_CHANNEL_WIDGET_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_CHANNEL_WIDGET))
#define GNOME_CHANNEL_WIDGET_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), GNOME_TYPE_CHANNEL_WIDGET, GnomeChannelWidgetClass))


typedef struct _GnomeChannelWidget	      GnomeChannelWidget;
typedef struct _GnomeChannelWidgetClass  GnomeChannelWidgetClass;
typedef struct _GnomeChannelWidgetPrivate GnomeChannelWidgetPrivate;

struct _GnomeChannelWidget
{
  GtkVBox parent_instance;

  GnomeChannelWidgetPrivate *private;
};

struct _GnomeChannelWidgetClass
{
  GtkVBoxClass parent_class;
};


GtkType	   gnome_channel_widget_get_type (void);
GtkWidget* gnome_channel_widget_new      (GnomeChannel *channel);

void gnome_channel_widget_place_in_table (GnomeChannelWidget *widget, GtkTable *table, int column);

G_END_DECLS

#endif /* GNOME_CHANNEL_WIDGET_H */

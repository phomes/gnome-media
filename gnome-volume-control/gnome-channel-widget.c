#include "gnome-channel-widget.h"

struct _GnomeChannelWidgetPrivate {
  gboolean made_change;
  gboolean was_changed;
  GnomeChannel *channel;

  GtkVScale *vscale;
  GtkLabel  *channel_label;
  GtkImage  *channel_image;
};

static void
volume_changed_cb (GnomeChannel *channel, float volume, GnomeChannelWidget *widget)
{
  if (!widget->private->made_change) {
    widget->private->was_changed = TRUE;
    gtk_range_set_value (GTK_RANGE (widget->private->vscale), 1.0f - volume);
  } else {
    widget->private->made_change = FALSE;
  }
}

static void 
slider_value_changed_cb (GtkVScale *vscale, GnomeChannelWidget *widget)
{
  if (!widget->private->was_changed) {
    widget->private->made_change = TRUE;
    gnome_channel_set_volume (widget->private->channel, 
			      1.0f - gtk_range_get_value (GTK_RANGE (vscale)),
			      NULL);
  } else {
    widget->private->was_changed = FALSE;
  }
}

static void
gnome_channel_widget_class_init (GnomeChannelWidgetClass *class)
{
}

static void
gnome_channel_widget_init (GnomeChannelWidget *widget)
{
  widget->private = g_new0 (GnomeChannelWidgetPrivate, 1);
}

void
gnome_channel_widget_place_in_table (GnomeChannelWidget *widget, GtkTable *table, int column)
{
  char *name;
  char *pixmap;

  if (widget->private->channel_image) {
    gtk_table_attach (GTK_TABLE (table), 
		      GTK_WIDGET (widget->private->channel_image),
		      column, column + 1, 1, 2, 
		      GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  }
  
  gtk_table_attach (GTK_TABLE (table), 
		    GTK_WIDGET (widget->private->channel_label),
		    column, column + 1, 2, 3, 
		    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
  
  gtk_table_attach (GTK_TABLE (table),
		    GTK_WIDGET (widget->private->vscale),
		    column, column + 1, 3, 4,
		    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);
}

GtkWidget*
gnome_channel_widget_new (GnomeChannel *channel)
{
  GtkTable *table;
  GnomeChannelWidget *widget;

  char *pixmap;
  char *name;

  table = GTK_TABLE (gtk_table_new (3, 1, FALSE));
  widget = gtk_type_new (gnome_channel_widget_get_type ());

  widget->private->made_change = FALSE;
  widget->private->was_changed = FALSE;
  widget->private->channel = channel;
  widget->private->vscale = GTK_VSCALE (gtk_vscale_new_with_range (0.0f, 1.0f, 0.1f));

  gtk_widget_set_size_request (GTK_WIDGET (widget->private->vscale), -1, 150);
  gtk_scale_set_draw_value (GTK_SCALE (widget->private->vscale), FALSE);
  gtk_range_set_value (GTK_RANGE (widget->private->vscale), 
		       1.0f - gnome_channel_get_volume (channel, NULL));


  pixmap = gnome_channel_get_pixmap (channel, NULL);
  if (pixmap != NULL) {
    widget->private->channel_image = GTK_IMAGE (gtk_image_new_from_stock (pixmap, GTK_ICON_SIZE_BUTTON)); 
  } else {
    widget->private->channel_image = NULL;
  }

  name = gnome_channel_get_name (channel, NULL);
  if (name != NULL) {
    widget->private->channel_label = GTK_LABEL (gtk_label_new (name));
  } else {
    widget->private->channel_label = GTK_LABEL (gtk_label_new (""));
  }

  /* gnome_channel_widget_place_in_table (widget, table, 1); */

  gtk_box_pack_start (GTK_BOX (widget), GTK_WIDGET (table),
		      TRUE, TRUE, 0);

  g_signal_connect (G_OBJECT (widget->private->vscale),
		    "value_changed",
		    G_CALLBACK (slider_value_changed_cb),
		    widget);
  g_signal_connect (G_OBJECT (channel),
		    "volume_changed",
		    G_CALLBACK (volume_changed_cb),
		    widget);

  return GTK_WIDGET (widget);
}

GtkType
gnome_channel_widget_get_type (void)
{
  static GtkType widget_type = 0;

  if (!widget_type)
    {
      static const GtkTypeInfo widget_info =
      {
	"GnomeChannelWidget",
	sizeof (GnomeChannelWidget),
	sizeof (GnomeChannelWidgetClass),
	(GtkClassInitFunc) gnome_channel_widget_class_init,
	(GtkObjectInitFunc) gnome_channel_widget_init,
	/* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };

      widget_type = gtk_type_unique (GTK_TYPE_VBOX, &widget_info);
    }

  return widget_type;
}


#include "gnome-mixer.h"
#include "oss-mixer.h"

#include "gnome-channel.h"
#include "gnome-channel-widget.h"
#include "gnome-channel-group.h"
#include "device-module.h"

#include <gtk/gtk.h>
#include <gnome.h>

#include "debug.h"

#include <gmodule.h>

#define GNOME_VOLUME_CONTROL_DEVICE_PATH "/gnome/GNOME2/lib/gnome-volume-control/devices"

static GnomeMixer *
get_mixer (void)
{
  static char *devices[] = {"oink", "alsa", "oss", NULL};
  int i;

  for (i = 0; devices[i] != NULL; i++) {
    GList *mixers;
    char *module_name;
    GetMixersFunc *get_mixers_func = NULL;
    GModule *module;
    gboolean success;

    module_name = g_module_build_path (GNOME_VOLUME_CONTROL_DEVICE_PATH,
				       devices[i]);
    
    module = g_module_open (module_name, 0 /*G_MODULE_BIND_LAZY*/);
    if (module == NULL) {
      g_warning ("Cannot load module `%s' (%s)", module_name, g_module_error ());
      continue;
    }
    
    success = g_module_symbol (module, "get_mixers",
			       (gpointer *) &get_mixers_func);  

    if ((!success) || get_mixers_func == NULL) {
      g_warning ("Could not load module `%s`, couldn't find mixers function", module_name);
      continue;
    }
    printf ("about to call\n");
    g_warning ("Errors? %s: %s", module_name, g_module_error ());
    mixers = (*get_mixers_func) (NULL);
    printf ("kablamo!\n");
    if (mixers == NULL) {
      continue;
    } else {
      return GNOME_MIXER (mixers->data);
    }
  }
  
}

int
main (int argc, char **argv)
{
  GnomeMixer *mixer;
  GList *channels;
  GList *node;
  GtkWidget *window;
  GtkWidget *table;
  GError *error = NULL;
  int i;

  GnomeChannel *channel;
  GtkWidget *channel_widget;

  gnome_init("gnome-volume-control", "0.1", argc, argv);

  make_warnings_and_criticals_stop_in_debugger ();

  gnome_window_icon_set_default_from_file (GNOME_ICONDIR"/gnome-volume-control.png");

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "Volume Control");

  table = gtk_table_new (3, 1, FALSE);

  gtk_container_add (GTK_CONTAINER (window), table);
  gtk_container_set_border_width (GTK_CONTAINER (table), 10);

  mixer = get_mixer();

  if (error != NULL) {
    g_warning ("Error %s\n", error->message);
    g_error_free (error);
  }

  channels = gnome_mixer_get_channels (mixer, NULL);

  i = 0;
  for (node = channels; node != NULL; node = node->next) {
    char *name, *pixmap;

    i++;

    channel = GNOME_CHANNEL (node->data);

    name = gnome_channel_get_name (channel, NULL);
    pixmap = gnome_channel_get_pixmap (channel, NULL);
    channel_widget = gnome_channel_widget_new (channel);

    gtk_table_resize (GTK_TABLE (table), 3, i);
    gnome_channel_widget_place_in_table (GNOME_CHANNEL_WIDGET (channel_widget), 
					 GTK_TABLE (table), i);
  }

  channel = GNOME_CHANNEL (gnome_channel_group_new (channels, "Foo", "none"));
  channel_widget = gnome_channel_widget_new (channel);
  gtk_table_resize (GTK_TABLE (table), 3, i + 1);
  gnome_channel_widget_place_in_table (GNOME_CHANNEL_WIDGET (channel_widget), 
				       GTK_TABLE (table), i + 1);

  gtk_table_set_col_spacings (GTK_TABLE (table), 10);

  gtk_widget_show_all (window);

  g_signal_connect (G_OBJECT (window), "delete_event",
		    gtk_main_quit, NULL);

  gtk_main();

  return 0;
}

/* GNOME Volume Control
 * Copyright (C) 2003-2004 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *
 * main.c: intialization, window setup
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <getopt.h>
#include <glib.h>
#include <gnome.h>
#include <gst/gst.h>
#include <gst/mixer/mixer.h>
#include <gst/propertyprobe/propertyprobe.h>

#include "keys.h"
#include "stock.h"
#include "window.h"

/*
 * Probe for mixer elements. Set up GList * with elements,
 * where each element has a GObject data node set of the
 * name "gnome-volume-control-name" with the value being
 * the human-readable name of the element.
 *
 * All elements in the returned GList * are in state
 * GST_STATE_NULL.
 */

static GList *
create_mixer_collection (void)
{
  const GList *elements;
  GList *collection = NULL;
  gint num = 0;

  /* go through all elements of a certain class and check whether
   * they implement a mixer. If so, add it to the list. */
  elements = gst_registry_pool_feature_list (GST_TYPE_ELEMENT_FACTORY);
  for ( ; elements != NULL; elements = elements->next) {
    GstElementFactory *factory = GST_ELEMENT_FACTORY (elements->data);
    gchar *title = NULL, *name;
    const gchar *klass;
    GstElement *element = NULL;
    const GParamSpec *devspec;
    GstPropertyProbe *probe;
    GValueArray *array = NULL;
    gint n;

    /* check category */
    klass = gst_element_factory_get_klass (factory);
    if (strcmp (klass, "Generic/Audio"))
      goto next;

    /* FIXME:
     * - maybe we want to rename the element to its actual name
     *     if we've found that?
     */
#define _label N_("Unknown Volume Control %d")

    /* create element */
    title = g_strdup_printf (gettext("Unknown Volume Control %d"), num);
    element = gst_element_factory_create (factory, title);
    if (!element)
      goto next;

    if (!GST_IS_PROPERTY_PROBE (element))
      goto next;

    probe = GST_PROPERTY_PROBE (element);
    if (!(devspec = gst_property_probe_get_property (probe, "device")))
      goto next;
    if (!(array = gst_property_probe_probe_and_get_values (probe, devspec)))
      goto next;

    /* set all devices and test for mixer */
    for (n = 0; n < array->n_values; n++) {
      GValue *device = g_value_array_get_nth (array, n);

      /* set this device */
      g_object_set_property (G_OBJECT (element), "device", device);
      if (gst_element_set_state (element,
				 GST_STATE_READY) == GST_STATE_FAILURE)
        continue;

      /* is this device a mixer? */
      if (!GST_IS_MIXER (element)) {
        gst_element_set_state (element, GST_STATE_NULL);
        continue;
      }

      /* any tracks? */
      if (!gst_mixer_list_tracks (GST_MIXER (element))) {
        gst_element_set_state (element, GST_STATE_NULL);
        continue;
      }

      /* fetch name */
      if (g_object_class_find_property (G_OBJECT_GET_CLASS (G_OBJECT (element)),
					"device-name")) {
        gchar *devname;
        g_object_get (element, "device-name", &devname, NULL);
        name = g_strdup_printf ("%s (%s)", devname,
				gst_element_factory_get_longname (factory));
      } else {
        name = g_strdup_printf ("%s (%s)", title,
				gst_element_factory_get_longname (factory));
      }
      g_object_set_data (G_OBJECT (element), "gnome-volume-control-name",
			 name);

      /* add to list */
      gst_element_set_state (element, GST_STATE_NULL);
      collection = g_list_append (collection, element);
      num++;

      /* and recreate this object, since we give it to the mixer */
      title = g_strdup_printf (gettext("Unknown Volume Control %d"), num);
      element = gst_element_factory_create (factory, title);
    }

next:
    if (element)
      gst_object_unref (GST_OBJECT (element));
    if (array)
      g_value_array_free (array);
    g_free (title);
  }

  return collection;
}

static void
register_stock_icons (void)
{
  GtkIconFactory *icon_factory;
  struct {
    gchar *filename, *stock_id;
  } list[] = {
    { "3dsound.png",      GNOME_VOLUME_CONTROL_STOCK_3DSOUND     },
    { "headphones.png",   GNOME_VOLUME_CONTROL_STOCK_HEADPHONES  },
    { "mixer.png",        GNOME_VOLUME_CONTROL_STOCK_MIXER       },
    { "noplay.png",       GNOME_VOLUME_CONTROL_STOCK_NOPLAY      },
    { "norecord.png",     GNOME_VOLUME_CONTROL_STOCK_NORECORD    },
    { "phone.png",        GNOME_VOLUME_CONTROL_STOCK_PHONE       },
    { "play.png",         GNOME_VOLUME_CONTROL_STOCK_PLAY        },
    { "record.png",       GNOME_VOLUME_CONTROL_STOCK_RECORD      },
    { "tone.png",         GNOME_VOLUME_CONTROL_STOCK_TONE        },
    { "video.png",        GNOME_VOLUME_CONTROL_STOCK_VIDEO       },
    { NULL, NULL }
  };
  gint num;

  icon_factory = gtk_icon_factory_new ();
  gtk_icon_factory_add_default (icon_factory);

  for (num = 0; list[num].filename != NULL; num++) {
    gchar *filename =
	gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_APP_PIXMAP,
				   list[num].filename, TRUE, NULL);

    if (filename) {
      GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file (filename, NULL);
      GtkIconSet *icon_set = gtk_icon_set_new_from_pixbuf (pixbuf);

      gtk_icon_factory_add (icon_factory, list[num].stock_id, icon_set);
      g_free (filename);
    }
  }
}

static void
cb_destroy (GtkWidget *widget,
	    gpointer   data)
{
  gtk_main_quit ();
}

void
cb_check_resize (GtkContainer    *container,
      		  gpointer         user_data)
{
  GConfClient *client;
  gint width, height;

  client = gconf_client_get_default();
  gtk_window_get_size (GTK_WINDOW (container), &width, &height);
  gconf_client_set_int (client, PREF_UI_WINDOW_WIDTH, width, NULL);
  gconf_client_set_int (client, PREF_UI_WINDOW_HEIGHT, height, NULL);
}

gint
main (gint   argc,
      gchar *argv[])
{
  gchar *appfile;
  GtkWidget *win;
  struct poptOption options[] = {
    { NULL, '\0', POPT_ARG_INCLUDE_TABLE, NULL, 0, "GStreamer", NULL },
    POPT_TABLEEND
  };
  GList *elements;

  /* i18n */
  bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  /* init gstreamer */
  options[0].arg = (void *) gst_init_get_popt_table ();

  /* init gtk/gnome */
  gnome_program_init ("gnome-volume-control", VERSION,
		      LIBGNOMEUI_MODULE, argc, argv,
		      GNOME_PARAM_POPT_TABLE, options,
		      GNOME_PARAM_APP_DATADIR, DATA_DIR, NULL);

   if (!gst_scheduler_factory_get_default_name ()) {
		GtkWidget *dialog;

		dialog = gtk_message_dialog_new (NULL,
						 0,
						 GTK_MESSAGE_ERROR,
						 GTK_BUTTONS_CLOSE,
						 _("Registry is not present or it is corrupted, please update it by running gst-register"));

		gtk_dialog_run (GTK_DIALOG (dialog));
		gtk_widget_destroy (dialog);

		exit (1);
	}

		

  /* init ourselves */
  register_stock_icons ();

  /* add appicon image */
  appfile = gnome_program_locate_file (NULL, GNOME_FILE_DOMAIN_APP_PIXMAP,
				       "mixer.png", TRUE,
				       NULL);
  if (appfile) {
    gnome_window_icon_set_default_from_file (appfile);
    g_free (appfile);
  }

  if (!(elements = create_mixer_collection ())) {
    win = gtk_message_dialog_new (NULL, 0, GTK_MESSAGE_ERROR,
				  GTK_BUTTONS_CLOSE,
				  _("No volume control elements and/or devices found."));
    gtk_widget_show (win);
    gtk_dialog_run (GTK_DIALOG (win));
    gtk_widget_destroy (win);
    return -1;
  }

  /* window contains everything automagically */
  win = gnome_volume_control_window_new (elements);
  g_signal_connect (win, "destroy", G_CALLBACK (cb_destroy), NULL);
  g_signal_connect (win, "check_resize", G_CALLBACK (cb_check_resize), NULL);

  gtk_widget_show (win);
  gtk_main ();

  return 0;
}

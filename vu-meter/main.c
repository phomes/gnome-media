/*
 * vu-meter -- A volume units meter for esd
 * Copyright (C) 1998 Gregory McLean
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
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 59 Temple Place - Suite 330, Cambridge, MA 
 * 02139, USA.
 *
 * Eye candy!
 */
#include <config.h>
#include <gnome.h>
#include <esd.h>
#include <gtkledbar.h>

gint sound = -1;
#define RATE   44100
/* A fairly good size buffer to keep resource (cpu) down */
#define NSAMP  2048
short          aubuf[NSAMP];
GtkWidget   *dial[2];
GtkWidget   *window;
void update (void);
gchar       *esd_host = NULL;
char 
*itoa (int i)
{
  static char ret[ 30 ];
  sprintf (ret, "%d", i);
  return ret;
}

static gint
save_state (GnomeClient *client, gint phase, GnomeRestartStyle save_style,
	    gint shutdown, GnomeInteractStyle inter_style, gint fast,
	    gpointer client_data)
{
  gchar  *argv[20];
  gchar  *session_id;
  gint   i;
  gint   xpos, ypos, w, h;

  session_id = gnome_client_get_id (client);
  gdk_window_get_geometry (window->window, &xpos, &ypos, &w, &h, NULL);
  gnome_config_push_prefix (gnome_client_get_config_prefix (client));
  gnome_config_set_int ("Geometry/x", xpos);
  gnome_config_set_int ("Geometry/y", ypos);
  gnome_config_set_int ("Geometry/w", w);
  gnome_config_set_int ("Geometry/h", h);
  gnome_config_pop_prefix ();
  gnome_config_sync();

  i    = 0;
  argv[i++] = (gchar *)client_data;
  argv[i++] = "-x";
  argv[i++] = g_strdup (itoa (xpos));
  argv[i++] = "-y";
  argv[i++] = g_strdup (itoa (ypos));
  gnome_client_set_restart_command (client, i, argv);
  gnome_client_set_clone_command (client, 0, NULL);
 
  g_free (argv[2]);
  g_free (argv[4]);
  return TRUE;
}

void 
open_sound (void)
{
  sound = esd_monitor_stream (ESD_BITS16|ESD_STEREO|ESD_STREAM|ESD_PLAY,
			      RATE, esd_host, "volume_meter");
  if (sound < 0)
    {
      g_error ("Cannot connect to EsoundD\n");
      exit (1);
    }
}

void 
update_levels (gpointer data, gint source, GdkInputCondition condition)
{
  register gint i;
  register short val_l, val_r;
  static unsigned short bigl, bigr;

  if (!(read (source, aubuf, NSAMP * 2)))
    return;
  bigl = bigr = 0;
  for (i = 1; i < NSAMP / 2;)
    {
      val_l = abs (aubuf[i++]);
      val_r = abs (aubuf[i++]);
      bigl = (val_l > bigl) ? val_l : bigl;
      bigr = (val_r > bigr) ? val_r : bigr;
    }
  bigl /= 256;
  bigr /= 256;
  led_bar_light_percent (dial[0], (bigl / 100.0));
  led_bar_light_percent (dial[1], (bigr / 100.0));

}


int 
main (int argc, char *argv[])
{
  GnomeClient   *client;
  GtkWidget     *hbox;
  GtkWidget     *frame;
  gint          i;
  gint          session_xpos = -1;
  gint          session_ypos = -1;
  gint          orient = 0;

  const struct poptOption options[] = 
  {
     { NULL, 'x', POPT_ARG_INT, &session_xpos, 0, 
       N_("Specify the X postion of the meter."), 
       N_("X-Position") },
     { NULL, 'y', POPT_ARG_INT, &session_ypos, 0, 
       N_("Specify the Y postion of the meter."), 
       N_("Y-Position") },
     { NULL, 's', POPT_ARG_STRING, &esd_host, 0, 
       N_("Connect to the esd server on this host."), 
       N_("ESD Server Host") },
     { NULL, 'v', POPT_ARG_NONE, &orient, 0, 
       N_("Open a vertical version of the meter."), NULL },
     { NULL, '\0', 0, NULL, 0 }
  };
  bindtextdomain (PACKAGE, GNOMELOCALEDIR);
  textdomain (PACKAGE);
  gnome_init_with_popt_table ("Volume Meter", "0.1", argc, argv, options, 
                              0, NULL);
  if (esd_host)
    g_print ("Host is %s\n", esd_host);
  client = gnome_master_client ();
  gtk_object_ref (GTK_OBJECT (client));
  gtk_object_sink (GTK_OBJECT (client));
  gtk_signal_connect (GTK_OBJECT (client), "save_yourself",
		      GTK_SIGNAL_FUNC (save_state), argv[0]);
  gtk_signal_connect (GTK_OBJECT (client), "die",
		      GTK_SIGNAL_FUNC (gtk_main_quit), argv[0]);
  if (GNOME_CLIENT_CONNECTED (client))
    {
      GnomeClient *cloned = gnome_cloned_client ();
      if (cloned)
	{
	  gnome_config_push_prefix (gnome_client_get_config_prefix (cloned));
	  session_xpos = gnome_config_get_int ("Geometry/x");
	  session_ypos = gnome_config_get_int ("Geometry/y");

	  gnome_config_pop_prefix ();
	}
    }
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "Volume Meter");
  if (session_xpos >=0 && session_ypos >= 0)
    gtk_widget_set_uposition (window, session_xpos, session_ypos);

  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
  gtk_signal_connect (GTK_OBJECT (window), "delete_event",
		      GTK_SIGNAL_FUNC (gtk_main_quit), NULL);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 4);
  gtk_container_add (GTK_CONTAINER (window), frame);

  if ( !orient ) 
      hbox = gtk_vbox_new (FALSE, 5);
  else
      hbox = gtk_hbox_new (FALSE, 5);
  gtk_container_border_width (GTK_CONTAINER (hbox), 5);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  for (i = 0; i < 2; i++)
    {
      dial[i] = led_bar_new (25, orient);
      gtk_box_pack_start (GTK_BOX (hbox), dial[i], FALSE, FALSE, 0);
    }
  gtk_widget_show_all (window);
  open_sound ();
  gdk_input_add (sound, GDK_INPUT_READ, update_levels, NULL);
  gtk_main ();
  gtk_object_unref (GTK_OBJECT (client));
  return 0;

}



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
#include <gnome.h>
#include <esd.h>
#include <gtkledbar.h>

gint sound = -1;
#define RATE   44100
/* A fairly good size buffer to keep resource (cpu) down */
#define NSAMP  2048
short          aubuf[NSAMP];
GtkWidget   *dial[2];
void update (void);

void 
open_sound (void)
{
  sound = esd_monitor_stream (ESD_BITS16|ESD_STEREO|ESD_STREAM|ESD_PLAY,
			      RATE, NULL, "volume_meter");
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
  GtkWidget     *window;
  GtkWidget     *hbox;
  GtkWidget     *frame;
  gint          i;

  gtk_init (&argc, &argv);
  
  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (window), "Volume Meter");
  gtk_signal_connect (GTK_OBJECT (window), "destroy",
		      GTK_SIGNAL_FUNC (gtk_main_quit), NULL);
  gtk_signal_connect (GTK_OBJECT (window), "delete_event",
		      GTK_SIGNAL_FUNC (gtk_main_quit), NULL);

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 4);
  gtk_container_add (GTK_CONTAINER (window), frame);
  hbox = gtk_vbox_new (FALSE, 5);
  gtk_container_border_width (GTK_CONTAINER (hbox), 5);
  gtk_container_add (GTK_CONTAINER (frame), hbox);
  for (i = 0; i < 2; i++)
    {
      dial[i] = led_bar_new (25);
      gtk_box_pack_start (GTK_BOX (hbox), dial[i], FALSE, FALSE, 0);
    }
  gtk_widget_show_all (window);
  open_sound ();
  gdk_input_add (sound, GDK_INPUT_READ, update_levels, NULL);
  gtk_main ();
}



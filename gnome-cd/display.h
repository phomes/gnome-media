/* 
 * display.h
 *
 * Copyright (C) 2001 Iain Holmes
 * Authors: Iain Holmes  <iain@ximian.com>
 */

#ifndef __DISPLAY_H__
#define __DISPLAY_H__

#define DISPLAY_LINE_TIME 0
#define DISPLAY_LINE_INFO 1
#define DISPLAY_LINE_ARTIST 2
#define DISPLAY_LINE_ALBUM 3
#define DISPLAY_LINE_TRACK 4

void display_size_allocate_cb (GtkWidget *drawing_area,
			       GtkAllocation *allocation,
			       GnomeCD *gcd);
gboolean display_expose_cb (GtkWidget *drawing_area,
			    GdkEventExpose *event,
			    GnomeCD *gcd);
int display_update_cb (gpointer data);

#endif

 

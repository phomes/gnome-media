#define TWIN
#include <gtk/gtk.h>
#include "gtv.h"
#include <signal.h>
#include <string.h>

static void tv_drag_request(GtkWidget *widget,
			    GdkEventDragRequest *event,
			    GtkTV *tv);

static void
tv_drag_setup(GtkWidget *widget, GtkTV *tv)
{
  char *tnms[] = {"file:ALL", "url:ALL"};
  gtk_widget_dnd_drag_set(widget, TRUE, (char **)&tnms, 2);
  gtk_signal_connect(GTK_OBJECT(widget), "drag_request_event",
		     GTK_SIGNAL_FUNC(tv_drag_request), tv);
}

static void
tv_drag_request(GtkWidget *widget,
		GdkEventDragRequest *event,
		GtkTV *tv)
{
  GdkImlibImage *g;
  char fn[64];
  snprintf(fn, sizeof(fn), "/tmp/tv%d.ppm", time(NULL));
  g_print("Now trying to grab the image\n");
  g = gtk_tv_grab_image(GTK_TV(tv), 320, 240);
  g_print("save returned %d on %p\n",
	  gdk_imlib_save_image(g, fn, NULL), g);
  gtk_widget_dnd_data_set(widget, (GdkEvent *)event, fn, strlen(fn)+1);
}

int main(int argc, char *argv[])
{
	GtkWidget *w, *tv, *eb;

	gtk_init(&argc, &argv);
	gdk_imlib_init();
	signal(SIGINT, (gpointer)gtk_main_quit);
	signal(SIGTERM, (gpointer)gtk_main_quit);
	w = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	eb = gtk_event_box_new();
	gtk_container_add(GTK_CONTAINER(w), eb);

	gtk_signal_connect(GTK_OBJECT(w), "delete_event", gtk_main_quit, NULL);

#ifdef TWIN
	tv = gtk_tv_new(0);
	gtk_tv_release_privileges();
#else
	tv = gtk_label_new("Die you loser gtk");
#endif
	g_assert(tv);
#ifdef TWIN
	g_print("TV has %d inputs\n",
		gtk_tv_get_num_inputs(GTK_TV(tv)));
#endif
	gtk_widget_set_usize(GTK_WIDGET(w), 320, 240);
	gtk_container_add(GTK_CONTAINER(eb), tv);
#ifdef TWIN
	gtk_tv_set_toplevel(GTK_TV(tv));
	tv_drag_setup(eb, GTK_TV(tv));
#endif
	gtk_widget_show(w);
	gtk_widget_show(eb);
	gtk_widget_show(tv);

#ifdef TWIN
	gtk_tv_set_sound(GTK_TV(tv),
			 0.5, 0.5, 0.5, VIDEO_AUDIO_MUTE, VIDEO_SOUND_STEREO);

	gtk_tv_set_format(GTK_TV(tv), GTK_TV_FORMAT_NTSC);
	gtk_tv_set_input(GTK_TV(tv), 1);
#endif

	g_assert(tv->window);

	gtk_main();

	gtk_widget_hide(tv);
	gtk_widget_hide(eb);
	gtk_widget_hide(w);

	return 0;
}

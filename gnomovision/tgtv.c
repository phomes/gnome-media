#include <gtk/gtk.h>
#include "gtv.h"
#include <signal.h>

static void tv_drag_request(GtkWidget *widget,
			    GdkEventDragRequest *event,
			    GtkTV *tv);

static void
tv_drag_setup(GtkWidget *widget, GtkTV *tv)
{
  char *tnms[] = {"file:image/ppm", "url:image/ppm"};
  gtk_widget_dnd_drag_set(widget, TRUE, &tnms, 2);
  gtk_signal_connect(GTK_OBJECT(widget), "drag_request_event",
		     GTK_SIGNAL_FUNC(tv_drag_request), tv);
}

static void
tv_drag_request(GtkWidget *widget,
		GdkEventDragRequest *event,
		GtkTV *tv)
{
  GdkImlibImage *g;
  g_print("Now trying to grab the image\n");
  g = gtk_tv_grab_image(GTK_TV(tv), 320, 240);
  g_print("save returned %d\n",
	  gdk_imlib_save_image(g, "/tmp/tv.ppm", NULL));
  gtk_widget_dnd_data_set(widget, (GdkEvent *)event, "/tmp/tv.ppm", strlen("/tmp/tv.ppm"));
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

	gtk_signal_connect(GTK_OBJECT(w), "delete_event", gtk_main_quit, NULL);

	tv = gtk_tv_new(0);
	gtk_tv_release_privileges();
	g_assert(tv);
	g_print("TV has %d inputs\n",
		gtk_tv_get_num_inputs(GTK_TV(tv)));
	gtk_widget_set_usize(GTK_WIDGET(tv), 640, 480);
	gtk_container_add(GTK_CONTAINER(eb), tv);
	gtk_container_add(GTK_CONTAINER(w), eb);
	gtk_tv_set_toplevel(GTK_TV(tv));
	tv_drag_setup(eb, GTK_TV(tv));
	gtk_widget_show(w);
	gtk_widget_show(eb);
	gtk_widget_show(tv);

	gtk_tv_set_sound(GTK_TV(tv),
			 0.5, 0.5, 0.5, VIDEO_AUDIO_MUTE, VIDEO_SOUND_STEREO);

	gtk_tv_set_format(GTK_TV(tv), GTK_TV_FORMAT_NTSC);
	gtk_tv_set_input(GTK_TV(tv), 1);

	gtk_main();

	gtk_widget_hide(tv);
	gtk_widget_hide(eb);
	gtk_widget_hide(w);

	return 0;
}

#include <gtk/gtk.h>
#include "gtv.h"
#include <signal.h>


int main(int argc, char *argv[])
{
	GtkWidget *w, *tv;
	GdkImlibImage *g;

	gtk_init(&argc, &argv);
	gdk_imlib_init();
	signal(SIGINT, (gpointer)gtk_main_quit);
	signal(SIGTERM, (gpointer)gtk_main_quit);
	w = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	gtk_signal_connect(GTK_OBJECT(w), "delete_event", gtk_main_quit, NULL);

	tv = gtk_tv_new(0);
	gtk_tv_release_privileges();
	g_assert(tv);
	g_print("TV has %d inputs\n",
		gtk_tv_get_num_inputs(GTK_TV(tv)));
	gtk_widget_set_usize(GTK_WIDGET(tv), 160, 120);
	gtk_container_add(GTK_CONTAINER(w), tv);
	gtk_tv_set_toplevel(GTK_TV(tv));
	gtk_widget_show(tv);
	gtk_widget_show(w);

	gtk_tv_set_sound(GTK_TV(tv),
			 0.5, 0.5, 0.5, VIDEO_AUDIO_MUTE, VIDEO_SOUND_STEREO);

	gtk_tv_set_input(GTK_TV(tv), 1);
	gtk_main();

	g_print("Now trying to grab the image\n");
	g = gtk_tv_grab_image(GTK_TV(tv), 320, 240);
	gdk_imlib_save_image_to_ppm(g, "tv.ppm");

	return 0;
}

#include <gnome.h>
#include "gtv.h"
#include <signal.h>
#include <string.h>

/* Global variable. Beware. :_) */
GtkTV *tv;

void show_preferences_dialog(void);

static void tv_drag_request(GtkWidget *widget,
			    GdkEventDragRequest *event);

static void
tv_drag_setup(GtkWidget *widget)
{
  char *tnms[] = {"file:ALL", "url:ALL"};
  gtk_widget_dnd_drag_set(widget, TRUE, (char **)&tnms, 2);
  gtk_signal_connect(GTK_OBJECT(widget), "drag_request_event",
		     GTK_SIGNAL_FUNC(tv_drag_request), tv);
}

static void
tv_drag_request(GtkWidget *widget,
		GdkEventDragRequest *event)
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

GnomeUIInfo menu_file[] = {
  GNOMEUIINFO_ITEM("Preferences", "Change the settings for the TV set",
		   show_preferences_dialog, NULL),
  GNOMEUIINFO_ITEM("Exit", "Close the TV set", gtk_main_quit, NULL),
  GNOMEUIINFO_END
};

GnomeUIInfo menu_help[] = {
  GNOMEUIINFO_HELP("tgtv"),
  GNOMEUIINFO_END
};

GnomeUIInfo menus[] = {
  GNOMEUIINFO_SUBTREE("File", &menu_file),
  GNOMEUIINFO_SUBTREE("Help", &menu_help),
  GNOMEUIINFO_END
};

int main(int argc, char *argv[])
{
	GtkWidget *w, *tv, *eb;

	gnome_init("tgtv", NULL, argc, argv, 0, NULL);

	signal(SIGINT, (gpointer)gtk_main_quit);
	signal(SIGTERM, (gpointer)gtk_main_quit);

	w = gnome_app_new("tgtv", "The GNOME TV");

	eb = gtk_event_box_new();
	gnome_app_create_menus(GNOME_APP(w), menus);
	gnome_app_set_contents(GNOME_APP(w), eb);

	gtk_signal_connect(GTK_OBJECT(w), "delete_event", gtk_main_quit, NULL);

	tv = gtk_tv_new(0);
	gtk_tv_release_privileges();
	g_assert(tv);
	g_print("TV has %d inputs\n",
		gtk_tv_get_num_inputs(GTK_TV(tv)));
	gtk_widget_set_usize(GTK_WIDGET(w), 320, 240);
	gtk_container_add(GTK_CONTAINER(eb), tv);
	gtk_tv_set_toplevel(GTK_TV(tv));
	tv_drag_setup(eb);
	gtk_widget_show(w);
	gtk_widget_show(eb);
	gtk_widget_show(tv);

	gtk_tv_set_sound(GTK_TV(tv),
			 0.5, 0.5, 0.5, VIDEO_AUDIO_MUTE, VIDEO_SOUND_STEREO);

	gtk_tv_set_format(GTK_TV(tv), GTK_TV_FORMAT_NTSC);
	gtk_tv_set_input(GTK_TV(tv), 0);
	gtk_tv_set_frequency(GTK_TV(tv), 8308);

	g_assert(tv->window);

	gtk_main();

	gtk_widget_hide(tv);
	gtk_widget_hide(eb);
	gtk_widget_hide(w);

	return 0;
}

/* Preferences dialog */

void
adjust_brightness(GtkAdjustment *adj, GtkTV *tv)
{
  gtk_tv_set_picture(tv, adj->value, -1, -1, -1, -1);
}

void
adjust_hue(GtkAdjustment *adj, GtkTV *tv)
{
  gtk_tv_set_picture(tv, -1, adj->value, -1, -1, -1);
}

void
adjust_colour(GtkAdjustment *adj, GtkTV *tv)
{
  gtk_tv_set_picture(tv, -1, -1, adj->value, -1, -1);
}

void
adjust_contrast(GtkAdjustment *adj, GtkTV *tv)
{
  gtk_tv_set_picture(tv, -1, -1, -1, adj->value, -1);
}

void
adjust_whiteness(GtkAdjustment *adj, GtkTV *tv)
{
  gtk_tv_set_picture(tv, -1, -1, -1, -1, adj->value);
}

void
save_settings(GtkWidget *pbox, gint button_num)
{
  /* brightness, hue, colour, contrast, whiteness */
  gnome_config_set_float("/tgtv/brightness", 0.0);
}

GtkWidget *
create_preferences_dialog(void)
{
  GtkWidget *tmp1, *tmp2, *tmp3, *tmp4;
  GtkWidget *prefs_dialog;
  GtkAdjustment *adj;
  GString *gstr = g_string_new(NULL);
  char *ctmp;
  int i;
  gboolean btmp;
  struct { char *name, *label; gpointer changefunc; } funclist[] = {
    {"brightness", "Brightness:", &adjust_brightness },
    {"hue", "Hue:", &adjust_hue },
    {"colour", "Colour:", &adjust_colour },
    {"contrast", "Contrast:", &adjust_contrast },
    {"whiteness", "Whiteness:", &adjust_whiteness },
    {NULL, NULL}
  };

  prefs_dialog = gnome_property_box_new();
  gtk_signal_connect(GTK_OBJECT(prefs_dialog), "apply",
		     GTK_SIGNAL_FUNC(save_settings), tv);
  gnome_dialog_close_hides(GNOME_DIALOG(prefs_dialog), TRUE);

  tmp1 = gtk_vbox_new(TRUE, 5);

  for(i = 0; funclist[i].label; i++) {
    gfloat initval;
    tmp2 = gtk_hbox_new(TRUE, 5);
    tmp3 = gtk_label_new(funclist[i].label);
    gtk_container_add(GTK_CONTAINER(tmp2), tmp3);

    g_string_sprintf(gstr, "/%s/%s=0.5", gnome_app_id,
		     funclist[i].name);
    ctmp = gnome_config_get_string(gstr->str);
    adj = GTK_ADJUSTMENT(gtk_adjustment_new(atof(ctmp), 0.0, 1.0, 0.1, 0.1, 1.0)); /* ? */
    gtk_signal_connect(GTK_OBJECT(adj), "changed",
		       GTK_SIGNAL_FUNC(funclist[i].changefunc), tv);
    tmp3 = gtk_hscrollbar_new(adj);
    gtk_container_add(GTK_CONTAINER(tmp2), tmp3);
    gtk_container_add(GTK_CONTAINER(tmp1), tmp2);
  }

  gtk_widget_show_all(tmp1);
  
  g_string_free(gstr, TRUE);
  return prefs_dialog;
}

void
show_preferences_dialog(void)
{
  /* I'd use a GnomePropertyConfigurator here, BUT that doesn't allow
     for setting things on the fly (i.e. seeing brightness changes
     instantly reflected)... */
  static GtkWidget *prefs_dialog = NULL;

  if(!prefs_dialog)
    prefs_dialog = create_preferences_dialog();

  gtk_widget_show(prefs_dialog);
}

/* GNOME Volume Control
 * Copyright (C) 2003-2004 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *
 * window.c: main window
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

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <gconf/gconf-client.h>

#include "keys.h"
#include "preferences.h"
#include "window.h"

G_DEFINE_TYPE (GnomeVolumeControlWindow, gnome_volume_control_window, GTK_TYPE_WINDOW)

static void
menu_item_select_cb (GtkMenuItem *proxy, GtkStatusbar *statusbar)
{
  GtkAction *action;
  char *message;

  action = g_object_get_data (G_OBJECT (proxy), "gtk-action");

  g_return_if_fail (action != NULL);

  g_object_get (G_OBJECT (action), "tooltip", &message, NULL);

  if (message) {
    gtk_statusbar_push (statusbar, 0, message);
    g_free (message);
  }
}

static void
menu_item_deselect_cb (GtkMenuItem *proxy, GtkStatusbar *statusbar)
{
  gtk_statusbar_pop (statusbar, 0);
}

static void
connect_proxy_cb (GtkUIManager *manager,
                  GtkAction *action,
                  GtkWidget *proxy,
                  GtkStatusbar *statusbar)
{
  if (GTK_IS_MENU_ITEM (proxy)) {
    g_signal_connect (proxy, "select", G_CALLBACK (menu_item_select_cb), statusbar);
    g_signal_connect (proxy, "deselect", G_CALLBACK (menu_item_deselect_cb), statusbar);
  }
}

void gnome_volume_control_window_set_page(GtkWidget *widget, const gchar *page)
{
  GnomeVolumeControlWindow *win = GNOME_VOLUME_CONTROL_WINDOW (widget);

  if (g_ascii_strncasecmp(page, "playback",8) == 0)
    gtk_notebook_set_current_page (GTK_NOTEBOOK (win->el), 0);
  else if (g_ascii_strncasecmp(page, "recording",9) == 0)
    gtk_notebook_set_current_page (GTK_NOTEBOOK (win->el), 1);
  else if (g_ascii_strncasecmp(page, "switches",9) == 0)
    gtk_notebook_set_current_page (GTK_NOTEBOOK (win->el), 2);
  else if (g_ascii_strncasecmp(page, "options",9) == 0)
    gtk_notebook_set_current_page (GTK_NOTEBOOK (win->el), 3);
  else /* default is "playback" */
    gtk_notebook_set_current_page (GTK_NOTEBOOK (win->el), 0);
}

static void
disconnect_proxy_cb (GtkUIManager *manager,
                     GtkAction *action,
                     GtkWidget *proxy,
                     GtkStatusbar *statusbar)
{
  if (GTK_IS_MENU_ITEM (proxy)) {
    g_signal_handlers_disconnect_by_func (proxy, G_CALLBACK (menu_item_select_cb), statusbar);
    g_signal_handlers_disconnect_by_func (proxy, G_CALLBACK (menu_item_deselect_cb), statusbar);
  }
}


/*
 * Menu actions.
 */

static void
cb_change (GtkToggleAction *action,
	   GnomeVolumeControlWindow *win)
{
  GConfValue *value;
  gchar *device_name;

  if (gtk_toggle_action_get_active (action) == FALSE)
    return;

  device_name = g_object_get_data (G_OBJECT (action), "device-name");

  value = gconf_value_new (GCONF_VALUE_STRING);
  gconf_value_set_string (value, device_name);
  gconf_client_set (win->client, GNOME_VOLUME_CONTROL_KEY_ACTIVE_ELEMENT, value, NULL);
  gconf_value_free (value);
}

static void
cb_exit (GtkAction *action,
	 GnomeVolumeControlWindow *win)
{
  gtk_widget_destroy (GTK_WIDGET (win));
}

static void
cb_preferences_destroy (GtkWidget *widget,
			GnomeVolumeControlWindow *win)
{
  win->prefs = NULL;
}

static void
cb_preferences (GtkAction *action,
		GnomeVolumeControlWindow *win)
{

  if (!win->prefs) {
    win->prefs = gnome_volume_control_preferences_new (GST_ELEMENT (win->el->mixer),
						       win->client);
    g_signal_connect (win->prefs, "destroy", G_CALLBACK (cb_preferences_destroy), win);
    gtk_widget_show (win->prefs);
  } else {
    gtk_window_present (GTK_WINDOW (win->prefs));
  }
}

static void 
open_uri (GtkWindow *parent, 
          const char *uri)
{
  GtkWidget *dialog;
  GdkScreen *screen;
  GError *error = NULL;
  gchar *cmdline;

  screen = gtk_window_get_screen (parent);

  cmdline = g_strconcat ("xdg-open ", uri, NULL);

  if (gdk_spawn_command_line_on_screen (screen, cmdline, &error) == FALSE) {
    dialog = gtk_message_dialog_new (parent, GTK_DIALOG_DESTROY_WITH_PARENT, 
                                     GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE, error->message);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    g_error_free(error);
  }
  g_free(cmdline);
}


static void
cb_help (GtkAction *action,
	 GnomeVolumeControlWindow *win)
{
  open_uri (GTK_WINDOW (win), "ghelp:gnome-volume-control");
}

static void
cb_about (GtkAction *action,
	  GnomeVolumeControlWindow *win)
{
  const gchar *authors[] = { "Ronald Bultje <rbultje@ronald.bitfreak.net>",
			     "Leif Johnson <leif@ambient.2y.net>",
			     NULL };
  const gchar *documenters[] = { "Sun Microsystems",
				 NULL};
  
  gtk_show_about_dialog (GTK_WINDOW (win),
			 "version", VERSION,
			 "copyright", "Copyright \xc2\xa9 2003-2004 Ronald Bultje",
			 "comments", _("A GNOME/GStreamer-based volume control application"),
			 "authors", authors,
			 "documenters", documenters,
			 "translator-credits", _("translator-credits"),
			 "logo-icon-name", "multimedia-volume-control",
			 NULL);
}

static void
cb_gconf (GConfClient *client,
	  guint        connection_id,
	  GConfEntry  *entry,
	  gpointer     data)
{
  GnomeVolumeControlWindow *win = GNOME_VOLUME_CONTROL_WINDOW (data);
  GConfValue *value;
  const gchar *el, *cur_el_str;

  if (!strcmp (gconf_entry_get_key (entry),
	       GNOME_VOLUME_CONTROL_KEY_ACTIVE_ELEMENT) &&
      (value = gconf_entry_get_value (entry)) != NULL &&
      (value->type == GCONF_VALUE_STRING) &&
      (el = gconf_value_get_string (value)) != NULL) {
    GList *item;

    for (item = win->elements; item != NULL; item = item->next) {
      cur_el_str = g_object_get_data (item->data, "gnome-volume-control-name");
      if (!strcmp (cur_el_str, el)) {
        GstElement *old_element = GST_ELEMENT (win->el->mixer);
        gchar *title;

        /* change element */
        gst_element_set_state (item->data, GST_STATE_READY);
        gnome_volume_control_element_change (win->el, item->data);
        if (win->prefs)
          gnome_volume_control_preferences_change (
		GNOME_VOLUME_CONTROL_PREFERENCES (win->prefs), item->data);
        gst_element_set_state (old_element, GST_STATE_NULL);

        /* change window title */
        title = g_strdup_printf (_("Volume Control: %s"), cur_el_str);
        gtk_window_set_title (GTK_WINDOW (win), title);
        g_free (title);

        break;
      }
    }
  }
}

/*
 * Signal handlers.
 */

#if 0
static void
cb_error (GstElement *element,
	  GstElement *source,
	  GError     *error,
	  gchar      *debug,
	  gpointer    data)
{
  GnomeVolumeControlWindow *win = GNOME_VOLUME_CONTROL_WINDOW (data);
  GtkWidget *dialog;

  dialog = gtk_message_dialog_new (GTK_WINDOW (win),
                                   GTK_DIALOG_DESTROY_WITH_PARENT,
                                   GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
                                   error->message);
  gtk_widget_show (dialog);
  gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_destroy (dialog);
}
#endif

static void
gnome_volume_control_window_dispose (GObject *object)
{
  GnomeVolumeControlWindow *win = GNOME_VOLUME_CONTROL_WINDOW (object);

  if (win->prefs) {
    gtk_widget_destroy (win->prefs);
    win->prefs = NULL;
  }

  /* clean up */
  if (win->elements) {
    const GList *item;

    g_list_foreach (win->elements, (GFunc) g_object_unref, NULL);
    g_list_free (win->elements);
    win->elements = NULL;
  }

  if (win->client) {
    g_object_unref (win->client);
    win->client = NULL;
  }

  G_OBJECT_CLASS (gnome_volume_control_window_parent_class)->dispose (object);
}


static void
gnome_volume_control_window_class_init (GnomeVolumeControlWindowClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = gnome_volume_control_window_dispose;
}


static void
gnome_volume_control_window_init (GnomeVolumeControlWindow *win)
{
  int width, height;

  win->elements = NULL;
  win->el = NULL;
  win->client = gconf_client_get_default ();
  win->prefs = NULL;
  win->use_default_mixer = FALSE;

  g_set_application_name (_("Volume Control"));
  gtk_window_set_title (GTK_WINDOW (win), _("Volume Control"));

  /* To set the window according to previous geometry */
  width = gconf_client_get_int (win->client, PREF_UI_WINDOW_WIDTH, NULL);
  if (width < 250)
    width = 250;
  height = gconf_client_get_int (win->client, PREF_UI_WINDOW_HEIGHT, NULL);
  if (height < 100)
    height = -1;
  gtk_window_set_default_size (GTK_WINDOW (win), width, height);
}


static const GtkActionEntry action_entries[] = {
  { "File",  NULL, N_("_File") },
  { "Edit",  NULL, N_("_Edit") },
  { "Help",  NULL, N_("_Help") },

  { "FileChangeDevice", NULL,  N_("_Change Device"), NULL, 
    N_("Control volume on a different device"),
    NULL },
  { "FileQuit", GTK_STOCK_QUIT, N_("_Quit"), "<control>Q",  
    N_("Quit the application"),
    G_CALLBACK (cb_exit) },
  { "EditPreferences", GTK_STOCK_PREFERENCES, N_("Prefere_nces"), NULL, 
    N_("Configure the application"), 
    G_CALLBACK (cb_preferences) },
  { "HelpContents", GTK_STOCK_HELP, N_("_Contents"), "F1", 
    N_("Help on this application"),
    G_CALLBACK (cb_help) },
  { "HelpAbout", GTK_STOCK_ABOUT, N_("_About"), NULL, 
    N_("About this application"),
    G_CALLBACK (cb_about) }
};



GtkWidget *
gnome_volume_control_window_new (GList *elements)
{
  gchar *active_el_str, *cur_el_str;
  GstElement *active_element;
  GList *item;
  GnomeVolumeControlWindow *win;
  GtkWidget *el;
  gint count = 0;
  gchar *title;
  guint change_device_menu_id;
  GtkActionGroup *action_group;
  GtkWidget *vbox;
  GtkWidget *menubar;
  GSList *radio_group = NULL;
  gint active_element_num;

  g_return_val_if_fail (elements != NULL, NULL);
  active_element = NULL;

  /* window */
  win = g_object_new (GNOME_VOLUME_CONTROL_TYPE_WINDOW, NULL);
  win->elements = elements;

  win->statusbar = GTK_STATUSBAR (gtk_statusbar_new ());

  win->ui_manager = gtk_ui_manager_new ();

  /* Hookup menu tooltips to the statusbar */
  g_signal_connect (win->ui_manager, "connect_proxy",
	            G_CALLBACK (connect_proxy_cb), win->statusbar);
  g_signal_connect (win->ui_manager, "disconnect_proxy",
		    G_CALLBACK (disconnect_proxy_cb), win->statusbar);

  action_group = gtk_action_group_new ("MenuActions");
  gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
	
  gtk_action_group_add_actions (action_group, action_entries, 
                                G_N_ELEMENTS (action_entries), win);

  gtk_ui_manager_insert_action_group (win->ui_manager, action_group, 0);

  gtk_ui_manager_add_ui_from_file (win->ui_manager, DATA_DIR "/gnome-volume-control-ui.xml", NULL);

  menubar = gtk_ui_manager_get_widget (win->ui_manager, "/MainMenu");



  /* get active element, if any (otherwise we use the default) */
  active_el_str = gconf_client_get_string (win->client,
					   GNOME_VOLUME_CONTROL_KEY_ACTIVE_ELEMENT,
					   NULL);
  if (active_el_str != NULL && active_el_str != '\0') {
    for (count = 0, item = elements; item != NULL; item = item->next, count++) {
      cur_el_str = g_object_get_data (item->data, "gnome-volume-control-name");
      if (!strcmp (active_el_str, cur_el_str)) {
        active_element = item->data;
        break;
      }
    }
    g_free (active_el_str);
    if (!item) {
      count = 0;
      active_element = elements->data;
      /* If there's a default but it doesn't match what we have available,
       * reset the default */
      gconf_client_set_string (win->client,
      			       GNOME_VOLUME_CONTROL_KEY_ACTIVE_ELEMENT,
      			       g_object_get_data (G_OBJECT (active_element),
      			       			  "gnome-volume-control-name"),
      			       NULL);
    }
    /* default element to first */
    if (!active_element)
      active_element = elements->data;
  } else {
    count = 0;
    active_element = elements->data;
  }
  active_element_num = count;

  change_device_menu_id = gtk_ui_manager_new_merge_id (win->ui_manager);

  for (count = 0, item = elements; item != NULL; item = item->next, count++) {
    const gchar *name;
    gchar *tip;
    gchar *label;
    GtkRadioAction *radio_action;

    name = g_object_get_data (item->data, "gnome-volume-control-name");
    tip = g_strdup_printf (_("Change device to %s"), name);
    label = g_strdup_printf ("_%d: %s", count, name);

    radio_action = gtk_radio_action_new (name, label, tip, NULL, count);
    g_object_set_data_full (G_OBJECT (radio_action), "device-name", 
                            g_strdup (name), (GDestroyNotify)g_free);

    g_free (tip);
    g_free (label);

    gtk_radio_action_set_group (radio_action, radio_group);
    radio_group = gtk_radio_action_get_group (radio_action);

    g_signal_connect (radio_action, "activate", G_CALLBACK (cb_change), win);

    if (count == active_element_num)
      gtk_radio_action_set_current_value (radio_action, active_element_num);

    gtk_action_group_add_action (action_group, GTK_ACTION (radio_action));
    g_object_unref (radio_action);

    gtk_ui_manager_add_ui (win->ui_manager, change_device_menu_id,
                           "/MainMenu/File/FileChangeDevice/Devices Placeholder",
                           name, name, GTK_UI_MANAGER_AUTO, FALSE);

  }


  /* gconf */
  gconf_client_add_dir (win->client, GNOME_VOLUME_CONTROL_KEY_DIR,
			GCONF_CLIENT_PRELOAD_RECURSIVE, NULL);
  gconf_client_notify_add (win->client, GNOME_VOLUME_CONTROL_KEY_DIR,
			   cb_gconf, win, NULL, NULL);

  /* window title and menu selection */
  title = g_strdup_printf (_("Volume Control: %s"),
			   g_object_get_data (G_OBJECT (active_element),
					      "gnome-volume-control-name"));
  gtk_window_set_title (GTK_WINDOW (win), title);
  g_free (title);

  win->use_default_mixer = (active_el_str == NULL);

  /* add content for this element */
  gst_element_set_state (active_element, GST_STATE_READY);
  el = gnome_volume_control_element_new (active_element,
					 win->client,
					 win->statusbar);
  win->el = GNOME_VOLUME_CONTROL_ELEMENT (el);
  gtk_container_set_border_width (GTK_CONTAINER (el), 6);

  /* Put the menubar, the elements and the statusbar in a vbox */
  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER(win), vbox);
  gtk_box_pack_start (GTK_BOX (vbox), menubar, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), el, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET(win->statusbar), FALSE, FALSE, 0);

  gtk_widget_show_all (GTK_WIDGET (win));

  /* FIXME:
   * - set error handler (cb_error) after device activation:
   *     g_signal_connect (element, "error", G_CALLBACK (cb_error), win);.
   * - on device change: reset error handler, change menu (in case this
   *     was done outside the UI).
   */

  return GTK_WIDGET (win);
}



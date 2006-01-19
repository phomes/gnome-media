/* -*- mode: c; style: linux -*- */
/* -*- c-basic-offset: 2 -*- */

/* gstreamer-properties.c
 * Copyright (C) 2002 Jan Schmidt
 * Copyright (C) 2005 Tim-Philipp Müller <tim centricular net>
 *
 * Written by: Jan Schmidt <thaytan@mad.scientist.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <locale.h>
#include <string.h>
#include <gnome.h>
#include <glade/glade.h>
#include <gtk/gtk.h>
#include <gconf/gconf-client.h>
#include <gst/gst.h>
#include "gstreamer-properties-structs.h"
#include "pipeline-tests.h"

#define WID(s) glade_xml_get_widget (interface_xml, s)
static GladeXML *interface_xml = NULL;
static GtkDialog *main_window;

static gchar pipeline_editor_property[] = "gstp-editor";
static gchar pipeline_desc_property[] = "gstp-pipe-desc";

static gchar *
gst_properties_gconf_get_full_key (const gchar * key)
{
  return g_strdup_printf ("/system/gstreamer/%d.%d/%s",
      GST_VERSION_MAJOR, GST_VERSION_MINOR, key);
}

gchar *
gst_properties_gconf_get_string (const gchar * key)
{
  GConfClient *client;
  GError *error = NULL;
  gchar *value = NULL;
  gchar *full_key;

  full_key = gst_properties_gconf_get_full_key (key);

  client = gconf_client_get_default ();

  value = gconf_client_get_string (client, full_key, &error);
  g_free (full_key);

  if (error) {
    g_warning ("%s() error: %s", __FUNCTION__, error->message);
    g_error_free (error);
    return NULL;
  }

  return value;
}

void
gst_properties_gconf_set_string (const gchar * key, const gchar * value)
{
  GConfClient *client;
  GError *error = NULL;
  gchar *full_key;

  full_key = gst_properties_gconf_get_full_key (key);
  client = gconf_client_get_default ();

  gconf_client_set_string (client, full_key, value, &error);
  g_free (full_key);

  if (error) {
    g_warning ("%s() error: %s", __FUNCTION__, error->message);
    g_error_free (error);
  }
}

static void
dialog_response (GtkDialog * widget, gint response_id, GladeXML * dialog)
{
  if (response_id == GTK_RESPONSE_HELP)
    gnome_help_display ("gstreamer-properties.xml", NULL, NULL);
  else
    gtk_main_quit ();
}

static void
test_button_clicked (GtkButton * button, gpointer user_data)
{
  GSTPPipelineEditor *editor = (GSTPPipelineEditor *) (user_data);
  GSTPPipelineDescription *pipeline_desc =
      editor->pipeline_desc + editor->cur_pipeline_index;
  if (pipeline_desc->is_custom) {
    GtkEntry *entry = editor->entry;

    pipeline_desc->pipeline = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));
  }

  user_test_pipeline (interface_xml, GTK_WINDOW (main_window), pipeline_desc);

  if (pipeline_desc->is_custom) {
    g_free (pipeline_desc->pipeline);
    pipeline_desc->pipeline = NULL;
  }
}

static void
update_from_option (GSTPPipelineEditor * editor,
    GSTPPipelineDescription * pipeline_desc)
{
  /* optionmenu changed, update the edit box, 
   * and the appropriate GConf key */
  /* FIXME g_return_if_fail(editor); */
  /* g_return_if_fail(pipeline_desc); */

  editor->cur_pipeline_index = pipeline_desc->index;

  if (pipeline_desc->is_custom == FALSE) {
    if (pipeline_desc->pipeline)
      gtk_entry_set_text (editor->entry, pipeline_desc->pipeline);
    gtk_widget_set_sensitive (GTK_WIDGET (editor->entry), FALSE);

    /* Update GConf */
    gst_properties_gconf_set_string (editor->gconf_key,
        pipeline_desc->pipeline);
  } else {
    gtk_widget_set_sensitive (GTK_WIDGET (editor->entry), TRUE);
  }
}

static void
set_menuitem_by_pipeline (GtkWidget * widget, gpointer data)
{
  GSTPPipelineEditor *editor = (GSTPPipelineEditor *) (data);

  if (GTK_IS_MENU_ITEM (widget)) {
    GtkMenuItem *mi = GTK_MENU_ITEM (widget);
    GSTPPipelineDescription *pipeline_desc =
        (GSTPPipelineDescription *) (g_object_get_data (G_OBJECT (mi),
            pipeline_desc_property));
    if (pipeline_desc == (editor->pipeline_desc + editor->cur_pipeline_index)) {
      GtkMenuShell *menu =
          GTK_MENU_SHELL (gtk_option_menu_get_menu (editor->optionmenu));
      gtk_option_menu_set_history (editor->optionmenu,
          g_list_index (menu->children, mi));
    }
  }
}

static void
update_from_gconf (GSTPPipelineEditor * editor, const gchar * pipeline_str)
{
  /* Iterate over the pipelines in the editor, and locate the one 
     matching this pipeline_str. If none, then use 'Custom' entry */
  int i = 0;
  gint custom_desc = -1;

  /* g_return_if_fail (editor != NULL); */

  editor->cur_pipeline_index = -1;
  for (i = 0; i < editor->n_pipeline_desc; i++) {
    GSTPPipelineDescription *pipeline_desc = editor->pipeline_desc + i;

    if (pipeline_desc->is_custom == TRUE) {
      custom_desc = i;
    } else if (!strcmp (pipeline_desc->pipeline, pipeline_str)) {
      editor->cur_pipeline_index = i;
      break;
    }
  }

  if (editor->cur_pipeline_index < 0) {
    editor->cur_pipeline_index = custom_desc;
    if (custom_desc >= 0) {
      gtk_entry_set_text (editor->entry, pipeline_str);
      if (pipeline_str == NULL || *pipeline_str == '\0')
        gtk_widget_set_sensitive (GTK_WIDGET (editor->test_button), FALSE);
    }
  }

  if (editor->cur_pipeline_index >= 0) {
    GtkMenu *menu = GTK_MENU (gtk_option_menu_get_menu (editor->optionmenu));

    gtk_container_foreach (GTK_CONTAINER (menu), set_menuitem_by_pipeline,
        editor);
    update_from_option (editor,
        editor->pipeline_desc + editor->cur_pipeline_index);
  }
}

static void
pipeline_option_changed (GtkOptionMenu * optionmenu, gpointer user_data)
{
  GSTPPipelineEditor *editor = (GSTPPipelineEditor *) (user_data);
  GtkMenu *menu = NULL;
  GtkMenuItem *mi = NULL;
  GSTPPipelineDescription *pipeline_desc = NULL;

  /* Determine which option changed, retrieve the pipeline desc,
   * and call update_from_option */
  menu = GTK_MENU (gtk_option_menu_get_menu (optionmenu));
  /*FIXME: g_return_if_fail (menu != NULL); */
  mi = GTK_MENU_ITEM (gtk_menu_get_active (menu));
  pipeline_desc =
      (GSTPPipelineDescription *) (g_object_get_data (G_OBJECT (mi),
          pipeline_desc_property));

  update_from_option (editor, pipeline_desc);
}

static void
entry_changed (GtkEditable * editable, gpointer user_data)
{
  GSTPPipelineEditor *editor = (GSTPPipelineEditor *) (user_data);
  const gchar *new_text = gtk_entry_get_text (GTK_ENTRY (editable));

  if (new_text == NULL || *new_text == '\0') {
    /* disable test button */
    gtk_widget_set_sensitive (GTK_WIDGET (editor->test_button), FALSE);
  } else {
    gtk_widget_set_sensitive (GTK_WIDGET (editor->test_button), TRUE);
  }
  /* Update GConf */
  gst_properties_gconf_set_string (editor->gconf_key, new_text);
}

static gboolean
element_available (const gchar * pipeline)
{
  gboolean res = FALSE;
  gchar *p, *first_space;

  if (pipeline == NULL || *pipeline == '\0')
    return FALSE;

  p = g_strdup (pipeline);

  g_strstrip (p);

  /* skip the check and pretend all is fine if it's something that does
   * not look like an element name (e.g. parentheses to signify a bin) */
  if (!g_ascii_isalpha (*p)) {
    g_free (p);
    return TRUE;
  }

  /* just the element name, no arguments */
  first_space = strchr (p, ' ');
  if (first_space != NULL)
    *first_space = '\0';

  /* check if element is available */
  res = gst_default_registry_check_feature_version (p, GST_VERSION_MAJOR,
      GST_VERSION_MINOR, 0);

  g_free (p);
  return res;
}

static GtkOptionMenu *
create_pipeline_menu (GladeXML * dialog, GSTPPipelineEditor * editor)
{
  GtkOptionMenu *option = NULL;
  gint i;
  GSTPPipelineDescription *pipeline_desc = editor->pipeline_desc;


  option = GTK_OPTION_MENU (WID (editor->optionmenu_name));
  if (option) {
    GtkMenu *menu = GTK_MENU (gtk_menu_new ());
    GtkMenuItem *mi = NULL;

    for (i = 0; i < editor->n_pipeline_desc; i++) {
      if (element_available (pipeline_desc[i].pipeline)) {
        GstElement *pipeline;
        GError *error = NULL;

        pipeline = gst_parse_launch (pipeline_desc[i].pipeline, &error);
        if (pipeline != NULL) {
          gst_object_unref (pipeline);
        }
        if (error != NULL) {
          g_error_free (error);
          continue;
        }
      } else if (pipeline_desc[i].pipeline != NULL) {
        /* FIXME: maybe we should show those in the
         * combo box, but make them insensitive? Or is
         * that more confusing than helpful for users? */
        g_message ("Skipping unavailable plugin '%s'",
            pipeline_desc[i].pipeline);
        continue;
      } else {
        /* This is probably the 'Custom' pipeline */
      }

      mi = GTK_MENU_ITEM (gtk_menu_item_new_with_label (_(pipeline_desc[i].
                  name)));
      pipeline_desc[i].index = i;
      g_object_set_data (G_OBJECT (mi), pipeline_desc_property,
          (gpointer) & pipeline_desc[i]);
      gtk_widget_show (GTK_WIDGET (mi));
      gtk_menu_shell_append (GTK_MENU_SHELL (menu), GTK_WIDGET (mi));
    }

    gtk_option_menu_set_menu (option, GTK_WIDGET (menu));
  }

  return option;
}

static void
init_pipeline_editor (GladeXML * dialog, GSTPPipelineEditor * editor)
{
  gchar *gconf_init_pipe = NULL;

  /* g_return_if_fail(editor != NULL); */

  editor->optionmenu = create_pipeline_menu (dialog, editor);
  editor->entry = GTK_ENTRY (WID (editor->entry_name));
  editor->test_button = GTK_BUTTON (WID (editor->test_button_name));

  /* g_return_if_fail (editor->entry && editor->optionmenu && editor->test_button); */
  if (!(editor->entry && editor->optionmenu && editor->test_button))
    return;

  g_object_set_data (G_OBJECT (editor->optionmenu), pipeline_editor_property,
      (gpointer) (editor));
  g_signal_connect (G_OBJECT (editor->optionmenu), "changed",
      (GCallback) pipeline_option_changed, (gpointer) (editor));
  g_object_set_data (G_OBJECT (editor->entry), pipeline_editor_property,
      (gpointer) (editor));
  g_signal_connect (G_OBJECT (editor->entry), "changed",
      (GCallback) entry_changed, (gpointer) (editor));
  g_object_set_data (G_OBJECT (editor->test_button), pipeline_editor_property,
      (gpointer) (editor));
  g_signal_connect (G_OBJECT (editor->test_button), "clicked",
      (GCallback) test_button_clicked, (gpointer) (editor));

  gconf_init_pipe = gst_properties_gconf_get_string (editor->gconf_key);

  if (gconf_init_pipe) {
    update_from_gconf (editor, gconf_init_pipe);
    g_free (gconf_init_pipe);
  }
}

void
create_dialog (void)
{
  int i = 0;
  GdkPixbuf *icon = NULL;

  for (i = 0; i < pipeline_editors_count; i++) {
    init_pipeline_editor (interface_xml, pipeline_editors + i);
  }

  main_window = GTK_DIALOG (WID ("gst_properties_dialog"));
  if (!main_window) {
    /* Fatal error */
    gnome_app_error (GNOME_APP (gnome_program_get ()),
        _("Failure instantiating main window"));
    return;
  }

  g_signal_connect (G_OBJECT (main_window),
      "response", (GCallback) dialog_response, interface_xml);
  icon =
      gdk_pixbuf_new_from_file (GSTPROPS_ICONDIR "/gstreamer-properties.png",
      NULL);
  if (icon) {
    gtk_window_set_icon (GTK_WINDOW (main_window), icon);
  } else {
    g_warning ("Error loading main window icon %s",
        GSTPROPS_ICONDIR "/gstreamer-properties.png\n");
  }
  gtk_widget_show (GTK_WIDGET (main_window));
}

int
main (int argc, char **argv)
{
  /* bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR); */
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

/* FIXME: remove when bumping requirements to 2.14 */
#ifdef GNOME_PARAM_GOPTION_CONTEXT
  if (1) {
    GOptionContext *ctx;
    GOptionGroup *group;

    ctx = g_option_context_new ("gstreamer-properties");
    g_option_context_add_group (ctx, gst_init_get_option_group ());

    gnome_program_init ("gstreamer-properties", VERSION,
        LIBGNOMEUI_MODULE, argc, argv,
        GNOME_PARAM_GOPTION_CONTEXT, ctx,
        GNOME_PARAM_APP_DATADIR, DATADIR, NULL);
  }
#else /* GNOME_PARAM_GOPTION_CONTEXT */
  gst_init (&argc, &argv);

  gnome_program_init ("gstreamer-properties", VERSION,
      LIBGNOMEUI_MODULE, argc, argv, GNOME_PARAM_APP_DATADIR, DATADIR, NULL);
#endif /* GNOME_PARAM_GOPTION_CONTEXT */

  /* FIXME: hardcode uninstalled path here */
  if (g_file_test ("gstreamer-properties.glade", G_FILE_TEST_EXISTS) == TRUE) {
    interface_xml = glade_xml_new ("gstreamer-properties.glade", NULL, NULL);
  } else if (g_file_test (GSTPROPS_GLADEDIR "/gstreamer-properties.glade",
          G_FILE_TEST_EXISTS) == TRUE) {
    interface_xml =
        glade_xml_new (GSTPROPS_GLADEDIR "/gstreamer-properties.glade", NULL,
        NULL);
  }

  if (!interface_xml) {
    GtkWidget *dialog;

    dialog = gtk_message_dialog_new (NULL,
        0,
        GTK_MESSAGE_ERROR,
        GTK_BUTTONS_CLOSE,
        _("Failed to load glade file; please check your installation."));

    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);

    exit (1);
  }

  create_dialog ();

  if (main_window)
    gtk_main ();

  return 0;
}

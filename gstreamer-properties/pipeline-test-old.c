/* -*- mode: c; style: linux -*- */
/* -*- c-basic-offset: 2 -*- */

/* pipeline-tests.c
 * Copyright (C) 2002 Jan Schmidt
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
#include <gst/gst.h>
#include <gst/gstautoplug.h>
#include <gst/gconf/gconf.h>

#include "pipeline-tests.h"
#define WID(s) glade_xml_get_widget (interface_xml, s)
static guint64 iterations = 0;
static GstElement *gst_test_pipeline;
static GstClock *s_clock;

/* User responded in the dialog */
static void
user_test_pipeline_response(GtkDialog * widget, gint response_id,
			    GladeXML * dialog)
{
    /* Close the window causing the test to end */
    gtk_widget_hide(GTK_WIDGET(widget));
}

static gboolean pipeline_test_idle_func(gpointer data)
{
    gboolean busy;
    busy = gst_bin_iterate(GST_BIN(data));
    iterations++;
    g_print(".");
    return busy;
}

/* Lifted from gconf.c in gst-libs */
/* go through a bin, finding the one pad that is unconnected in the given
 *  * direction, and return that pad */
static GstPad *
gst_bin_find_unconnected_pad (GstBin *bin, GstPadDirection direction)
{
  GstPad *pad = NULL;
  GList *elements = NULL;
  const GList *pads = NULL;
  GstElement *element = NULL;

  elements = (GList *) gst_bin_get_list (bin);
  /* traverse all elements looking for unconnected pads */
  while (elements && pad == NULL)
  {
    element = GST_ELEMENT (elements->data);
    pads = gst_element_get_pad_list (element);
    while (pads)
    {
      /* check if the direction matches */
      if (GST_PAD_DIRECTION (GST_PAD (pads->data)) == direction)
      {
        if (GST_PAD_PEER (GST_PAD (pads->data)) == NULL)
        {
          /* found it ! */
          pad = GST_PAD (pads->data);
        }
      }
      if (pad) break; /* found one already */
      pads = g_list_next (pads);
    }
    elements = g_list_next (elements);
  }
  return pad;
}

static GstElement *autoplug_partial_bins(GstBin *src_bin, GstBin *sink_bin)
{
  GstElement *new_bin = NULL;
  GstAutoplug *autoplug = NULL;
  GstPad *src_pad = NULL;
  GstPad *sink_pad = NULL;
  GstCaps *caps = NULL;

  if ((src_pad = gst_bin_find_unconnected_pad (GST_BIN (src_bin), GST_PAD_SRC))){
    gst_element_add_ghost_pad (GST_ELEMENT(src_bin), src_pad, "src");
  }
  if ((sink_pad = gst_bin_find_unconnected_pad (GST_BIN (sink_bin), GST_PAD_SINK))){
    gst_element_add_ghost_pad (GST_ELEMENT(sink_bin), sink_pad, "sink");
  }
  g_return_val_if_fail(sink_pad && src_pad, NULL);

  autoplug = gst_autoplug_factory_make ("staticrender");
  g_return_val_if_fail(autoplug != NULL, NULL);

  caps = gst_pad_get_caps (GST_PAD(src_pad));
  g_return_val_if_fail(caps != NULL, NULL);

  new_bin = gst_autoplug_to_renderers(autoplug,
                                   caps,
                                   GST_ELEMENT(sink_bin), NULL);
  gst_object_unref(GST_OBJECT(autoplug));  
  return new_bin;
}

/* Build the pipeline */
static gboolean
build_test_pipeline(GSTPPipelineDescription * pipeline_desc)
{
  gboolean return_val = FALSE;
  GError *error = NULL;
  GstElement *cur_pipe_bin = NULL;
  GstElement *test_bin = NULL;
  gchar* test_pipeline_str = NULL;
  gchar* full_pipeline_str = NULL;

  switch (pipeline_desc->test_type) {
    case TEST_PIPE_AUDIOSINK:
      test_pipeline_str = gst_gconf_get_string ("default/audiosink");
      break;
    case TEST_PIPE_VIDEOSINK:
      test_pipeline_str = gst_gconf_get_string ("default/videosink");
      break;
    case TEST_PIPE_SUPPLIED:
      test_pipeline_str = pipeline_desc->test_pipe;
      break;
  }
  switch (pipeline_desc->type) {
    case PIPE_TYPE_AUDIOSINK:
    case PIPE_TYPE_VIDEOSINK:
      
      break;
    case PIPE_TYPE_AUDIOSRC:
    case PIPE_TYPE_VIDEOSRC:
      break;
  }
  return return_val;

  cur_pipe_bin = (GstElement *)gst_parse_launch(pipeline_desc->pipeline, &error);
  g_assert(cur_pipe_bin != NULL);
  if (error) {
    /* FIXME display the error? */
    g_error_free(error);
  }

  switch (pipeline_desc->test_type) {
    case TEST_PIPE_AUDIOSINK:
      test_bin = gst_gconf_get_default_audio_sink();
      break;
    case TEST_PIPE_VIDEOSINK:
      test_bin = gst_gconf_get_default_video_sink();
      break;
    case TEST_PIPE_SUPPLIED:
      g_print("creating test_bin '%s'\n", pipeline_desc->test_pipe);
      test_bin = (GstElement *)gst_parse_launch(pipeline_desc->test_pipe, &error);
      g_print("test_bin = %x, error = %x\n", test_bin, error);
      break;
  }
  g_assert(test_bin != NULL);
  if (error) {
    /* FIXME display the error? */
    g_error_free(error);
  }

  if (cur_pipe_bin && test_bin) {
    switch (pipeline_desc->type) {
      case PIPE_TYPE_AUDIOSINK:
      case PIPE_TYPE_VIDEOSINK:
        /* Autoplug the test_bin into the cur_pipe_bin */
        gst_test_pipeline = autoplug_partial_bins(GST_BIN(test_bin), GST_BIN(cur_pipe_bin));
        break;
      case PIPE_TYPE_AUDIOSRC:
      case PIPE_TYPE_VIDEOSRC:
        /* Autoplug the cur_pipe_bin into the test_bin */
        gst_test_pipeline = autoplug_partial_bins(GST_BIN(cur_pipe_bin), GST_BIN(test_bin));
        break;
    }
  }
  if (gst_test_pipeline)
      return_val = TRUE;

  if (!return_val) {
    if (cur_pipe_bin)
      gst_object_unref(GST_OBJECT(cur_pipe_bin));
    if (test_bin)
      gst_object_unref(GST_OBJECT(test_bin));
  }
  return return_val;
}

static void
pipeline_error_dlg(GtkWindow * parent,
		   GSTPPipelineDescription * pipeline_desc)
{
  gchar *errstr = g_strdup_printf( _("Failed to construct test pipeline for '%s'"),
		  pipeline_desc->name);
    if (parent == NULL) {
	    g_error(errstr);
    }
    else {
      GtkDialog *dialog = GTK_DIALOG(gtk_message_dialog_new(parent,
							  GTK_DIALOG_DESTROY_WITH_PARENT,
							  GTK_MESSAGE_ERROR,
							  GTK_BUTTONS_CLOSE,
							  errstr));
      gtk_dialog_run(GTK_DIALOG(dialog));
      gtk_widget_destroy(GTK_WIDGET(dialog));
    }
    g_free(errstr);
}

/* Construct and iterate the pipeline. Use the indicated parent
   for any user interaction window.
*/
void
user_test_pipeline(GladeXML * interface_xml,
		   GtkWindow * parent,
		   GSTPPipelineDescription * pipeline_desc)
{
  GtkDialog *dialog = NULL;
  gst_test_pipeline = NULL;
  s_clock = NULL;
  iterations = 0;

  /* Build the pipeline */
  if (!build_test_pipeline(pipeline_desc)) {
    /* Show the error pipeline */
    pipeline_error_dlg(parent, pipeline_desc);
    return;
  }

  /* Setup the 'click ok when done' dialog */
  if (parent) {
    dialog = GTK_DIALOG(WID("test_pipeline"));
    g_return_if_fail(dialog != NULL);
    gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
    g_signal_connect(G_OBJECT(dialog), "response",
         (GCallback) user_test_pipeline_response,
         interface_xml);
  }

  /* Start the pipeline */
  if (gst_element_set_state(gst_test_pipeline, GST_STATE_PLAYING) !=
    GST_STATE_SUCCESS) {
    pipeline_error_dlg(parent, pipeline_desc);
    return;
  }

  s_clock = gst_bin_get_clock(GST_BIN(gst_test_pipeline));
  if (!GST_FLAG_IS_SET(GST_OBJECT(gst_test_pipeline), 
       GST_BIN_SELF_SCHEDULABLE)) {
    g_idle_add(pipeline_test_idle_func, gst_test_pipeline);
  } else {
    gst_element_wait_state_change(gst_test_pipeline);
  }

  /* Show the dialog */
  if (dialog)
    gtk_widget_show(GTK_WIDGET(dialog));
  else {
    gboolean busy;
    guint64 max_iterations = 1000; /* A bit hacky: No parent dialog, run in limited test mode */
    do {
      busy = gst_bin_iterate(GST_BIN(gst_test_pipeline)) && (iterations < max_iterations);
      iterations++;
    } while (busy);
  }
  gst_element_set_state(gst_test_pipeline, GST_STATE_NULL);

  /* Free up the pipeline */
  gst_object_unref(GST_OBJECT(gst_test_pipeline));

  g_print("Done test. returning\n");
}

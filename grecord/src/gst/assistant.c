/* GStreamer Recorder
 * (c) 2003 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *
 * assistant.c: handle actual events from managing bin
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

#include "assistant.h"

/* GObject init */
static void gst_rec_assistant_class_init (GstRecAssistantClass * klass);
static void gst_rec_assistant_base_init (GstRecAssistantClass * klass);
static void gst_rec_assistant_init (GstRecAssistant * assistant);

/* proxying data */
static void gst_rec_assistant_chain (GstPad * pad, GstData * data);

/* the event handler - it's all about him */
static gboolean gst_rec_assistant_src_event (GstPad * pad, GstEvent * event);

/* keep track of state for reset */
static GstElementStateReturn
gst_rec_assistant_change_state (GstElement * element);

static GstElementClass *parent_class = NULL;

GType
gst_rec_assistant_get_type (void)
{
  static GType gst_rec_assistant_type = 0;

  if (!gst_rec_assistant_type) {
    static const GTypeInfo gst_rec_assistant_info = {
      sizeof (GstRecAssistantClass),
      (GBaseInitFunc) gst_rec_assistant_base_init,
      NULL,
      (GClassInitFunc) gst_rec_assistant_class_init,
      NULL,
      NULL,
      sizeof (GstRecAssistant),
      0,
      (GInstanceInitFunc) gst_rec_assistant_init,
      NULL
    };

    gst_rec_assistant_type =
	g_type_register_static (GST_TYPE_ELEMENT,
	"GstRecAssistant", &gst_rec_assistant_info, 0);
  }

  return gst_rec_assistant_type;
}

static void
gst_rec_assistant_base_init (GstRecAssistantClass * klass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);
  static GstElementDetails gst_rec_assistant_details =
      GST_ELEMENT_DETAILS ("Managing bin assistant",
      "Generic",
      "Manages private gst-rec events forwarded from the containing bin",
      "Ronald Bultje <rbultje@ronald.bitfreak.net>");

  gst_element_class_set_details (element_class, &gst_rec_assistant_details);
}

static void
gst_rec_assistant_class_init (GstRecAssistantClass * klass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);

  parent_class = g_type_class_ref (GST_TYPE_ELEMENT);

  element_class->change_state = gst_rec_assistant_change_state;
}

static void
gst_rec_assistant_init (GstRecAssistant * assistant)
{
  GST_FLAG_SET (assistant, GST_ELEMENT_EVENT_AWARE);

  assistant->sinkpad = gst_pad_new ("sink", GST_PAD_SINK);
  gst_pad_set_chain_function (assistant->sinkpad, gst_rec_assistant_chain);
  gst_pad_set_link_function (assistant->sinkpad, gst_pad_proxy_pad_link);
  gst_pad_set_getcaps_function (assistant->sinkpad, gst_pad_proxy_getcaps);
  gst_element_add_pad (GST_ELEMENT (assistant), assistant->sinkpad);

  assistant->srcpad = gst_pad_new ("src", GST_PAD_SRC);
  gst_pad_set_event_function (assistant->srcpad, gst_rec_assistant_src_event);
  gst_pad_set_link_function (assistant->srcpad, gst_pad_proxy_pad_link);
  gst_pad_set_getcaps_function (assistant->srcpad, gst_pad_proxy_getcaps);
  gst_element_add_pad (GST_ELEMENT (assistant), assistant->srcpad);

  assistant->eos = FALSE;
}

static void
gst_rec_assistant_chain (GstPad * pad, GstData * data)
{
  GstBuffer *buf;
  GstRecAssistant *assistant = GST_REC_ASSISTANT (gst_pad_get_parent (pad));

  if (GST_IS_EVENT (data)) {
    gst_pad_event_default (pad, GST_EVENT (data));
    return;
  }

  buf = GST_BUFFER (data);

  /* are we supposed to EOS? */
  if (assistant->eos) {
    gst_buffer_unref (buf);
    gst_pad_event_default (pad, gst_event_new (GST_EVENT_EOS));
    GST_DEBUG_OBJECT (assistant, "Signalling EOS on request");
  } else {
    gst_pad_push (assistant->srcpad, data);
  }
}

static gboolean
gst_rec_assistant_src_event (GstPad * pad, GstEvent * event)
{
  GstRecAssistant *assistant = GST_REC_ASSISTANT (gst_pad_get_parent (pad));
  gboolean res = FALSE;

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_EOS:
      assistant->eos = TRUE;
      res = TRUE;
      break;
    default:
      break;
  }

  gst_event_unref (event);

  return res;
}

static GstElementStateReturn
gst_rec_assistant_change_state (GstElement * element)
{
  GstRecAssistant *assistant = GST_REC_ASSISTANT (element);

  switch (GST_STATE_TRANSITION (element)) {
    case GST_STATE_PAUSED_TO_READY:
      assistant->eos = FALSE;
      break;
    default:
      break;
  }

  if (GST_ELEMENT_CLASS (parent_class)->change_state)
    return GST_ELEMENT_CLASS (parent_class)->change_state (element);

  return GST_STATE_SUCCESS;
}

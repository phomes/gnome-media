/* GStreamer Recorder
 * (c) 2003 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *
 * assistant.h: child of a managing bin, the actual event handler
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

#ifndef __GST_REC_ASSISTANT_H__
#define __GST_REC_ASSISTANT_H__

#include <gst/gst.h>

#define GST_REC_TYPE_ASSISTANT \
  (gst_rec_assistant_get_type())
#define GST_REC_ASSISTANT(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_REC_TYPE_ASSISTANT, \
			      GstRecAssistant))
#define GST_REC_ASSISTANT_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), GST_REC_TYPE_ASSISTANT, \
			   GstRecAssistantClass))
#define GST_REC_IS_ASSISTANT(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_REC_TYPE_ASSISTANT))
#define GST_REC_IS_ASSISTANT_CLASS(obj) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), GST_REC_TYPE_ASSISTANT))

typedef struct _GstRecAssistant
{
  GstElement parent;

  /* data pads */
  GstPad *sinkpad, *srcpad;

  /* are we supposed to emit EOS next? */
  gboolean eos;
} GstRecAssistant;

typedef struct _GstRecAssistantClass
{
  GstElementClass parent;
} GstRecAssistantClass;

GType gst_rec_assistant_get_type (void);

#endif /* __GST_REC_ASSISTANT_H__ */

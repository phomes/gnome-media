/* GStreamer Recorder
 * (c) 2003 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *
 * manager.h: managing bin. Handles private add-on events
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

#ifndef __GST_REC_MANAGER_H__
#define __GST_REC_MANAGER_H__

#include <gst/gst.h>

#define GST_REC_TYPE_MANAGER \
  (gst_rec_manager_get_type())
#define GST_REC_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), GST_REC_TYPE_MANAGER, \
			      GstRecManager))
#define GST_REC_MANAGER_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), GST_REC_TYPE_MANAGER, \
			   GstRecManagerClass))
#define GST_REC_IS_MANAGER(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), GST_REC_TYPE_MANAGER))
#define GST_REC_IS_MANAGER_CLASS(obj) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), GST_REC_TYPE_MANAGER))

typedef struct _GstRecManager
{
  GstBin parent;

  /* ghost pad for the last element's src pad */
  GstPad *srcpad;

  /* our assistant */
  GstElement *assistant, *src;
} GstRecManager;

typedef struct _GstRecManagerClass
{
  GstBinClass parent;
} GstRecManagerClass;

GType gst_rec_manager_get_type (void);

/* init stuff */
void gst_rec_elements_init (void);

#endif /* __GST_REC_MANAGER_H__ */

/* GStreamer Recorder
 * (c) 2003 Ronald Bultje <rbultje@ronald.bitfreak.net>
 *
 * manager.c: managing bin.
 *
 * This element will handle private events, such as EOS halfway
 * a stream, seeks which aren't anything else than a timestamp
 * shift (on elements not supporting seek) and some more. This
 * can be considered an application-extension to GStreamer events,
 * outside it's original goal, and thus not useful inside gst-core.
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
#include "manager.h"

static void gst_rec_manager_class_init (GstRecManagerClass * klass);
static void gst_rec_manager_base_init (GstRecManagerClass * klass);
static void gst_rec_manager_init (GstRecManager * manager);
static void gst_rec_manager_dispose (GObject * obj);

static void gst_rec_manager_child_eos (GstElement * element, gpointer data);

static void gst_rec_manager_child_add (GstBin * bin, GstElement * child);
static void gst_rec_manager_child_del (GstBin * bin, GstElement * child);

static GstBinClass *parent_class = NULL;

GType
gst_rec_manager_get_type (void)
{
  static GType gst_rec_manager_type = 0;

  if (!gst_rec_manager_type) {
    static const GTypeInfo gst_rec_manager_info = {
      sizeof (GstRecManagerClass),
      (GBaseInitFunc) gst_rec_manager_base_init,
      NULL,
      (GClassInitFunc) gst_rec_manager_class_init,
      NULL,
      NULL,
      sizeof (GstRecManager),
      0,
      (GInstanceInitFunc) gst_rec_manager_init,
      NULL
    };

    gst_rec_manager_type =
	g_type_register_static (GST_TYPE_BIN,
	"GstRecManager", &gst_rec_manager_info, 0);
  }

  return gst_rec_manager_type;
}

static void
gst_rec_manager_base_init (GstRecManagerClass * klass)
{
  GstElementClass *element_class = GST_ELEMENT_CLASS (klass);
  static GstElementDetails gst_rec_manager_details =
      GST_ELEMENT_DETAILS ("Managing bin",
      "Generic/Bin",
      "Manages private gst-rec events for the contained element",
      "Ronald Bultje <rbultje@ronald.bitfreak.net>");

  gst_element_class_set_details (element_class, &gst_rec_manager_details);
}

static void
gst_rec_manager_class_init (GstRecManagerClass * klass)
{
  GstBinClass *bin_class = GST_BIN_CLASS (klass);
  GObjectClass *obj_class = G_OBJECT_CLASS (klass);

  parent_class = g_type_class_ref (GST_TYPE_BIN);

  bin_class->add_element = gst_rec_manager_child_add;
  bin_class->remove_element = gst_rec_manager_child_del;
  obj_class->dispose = gst_rec_manager_dispose;
}

static void
gst_rec_manager_init (GstRecManager * manager)
{
  GstElement *assistant;
  GstPad *pad;
  static guint num = 0;
  gchar *name;

  name = g_strdup_printf ("event-handler-%u", num++);
  assistant = gst_element_factory_make ("assistant", name);
  g_free (name);
  pad = gst_element_get_pad (assistant, "src");
  gst_bin_add (GST_BIN (manager), assistant);
  g_signal_connect (G_OBJECT (assistant), "eos",
      G_CALLBACK (gst_rec_manager_child_eos), manager);

  manager->srcpad = gst_ghost_pad_new ("src", pad);
  gst_element_add_pad (GST_ELEMENT (manager), manager->srcpad);

  manager->assistant = assistant;
}

static void
gst_rec_manager_dispose (GObject * obj)
{
  GstRecManager *manager = GST_REC_MANAGER (obj);

  if (manager->src) {
    gst_bin_remove (GST_BIN (obj), manager->src);
  }

  G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
gst_rec_manager_child_eos (GstElement * element, gpointer data)
{
  gst_element_set_eos (GST_ELEMENT (data));
}

static void
gst_rec_manager_child_add (GstBin * bin, GstElement * child)
{
  GstRecManager *manager = GST_REC_MANAGER (bin);
  guint length = g_list_length ((GList *) gst_bin_get_list (bin));

  /* Right after creation, the first element to be added is the
   * assistant. This may never be removed until disposal. After
   * that, one child can be added (no more than one!). */
  g_assert (length < 2);

  if (length == 0)
    g_assert (G_OBJECT_TYPE (child) == GST_REC_TYPE_ASSISTANT);
  else {			/* length == 1 */

    g_assert (G_OBJECT_TYPE (child) != GST_REC_TYPE_ASSISTANT);

    /* link pad with assistant */
    gst_element_link (child, manager->assistant);
    manager->src = child;
  }

  if (parent_class->add_element)
    parent_class->add_element (bin, child);
}

static void
gst_rec_manager_child_del (GstBin * bin, GstElement * child)
{
  GstRecManager *manager = GST_REC_MANAGER (bin);
  guint length = g_list_length ((GList *) gst_bin_get_list (bin));

  /* One element (not the assistant) may be removed. Upon disposal,
   * but only then, the assistant may be removed. */
  g_assert (length <= 2);

  if (length == 1)
    g_assert (G_OBJECT_TYPE (child) == GST_REC_TYPE_ASSISTANT);
  else {			/* length == 2 */

    g_assert (G_OBJECT_TYPE (child) != GST_REC_TYPE_ASSISTANT);

    /* unlink pad with assistant */
    gst_element_unlink (child, manager->assistant);
    manager->src = NULL;
  }

  if (parent_class->remove_element)
    parent_class->remove_element (bin, child);
}

static gboolean
gst_rec_register_elements (GstPlugin * plugin)
{
  return (gst_element_register (plugin, "manager",
	  GST_RANK_NONE,
	  GST_REC_TYPE_MANAGER) &&
      gst_element_register (plugin, "assistant",
	  GST_RANK_NONE, GST_REC_TYPE_ASSISTANT));
}

static GstPluginDesc plugin_desc = {
  GST_VERSION_MAJOR,
  GST_VERSION_MINOR,
  "gst_rec_elements",
  "Private elements of " PACKAGE_NAME,
  gst_rec_register_elements,
  NULL,
  VERSION,
  "LGPL",
  PACKAGE,
  "http://www.gstreamer.net/",
  GST_PADDING_INIT
};

void
gst_rec_elements_init (void)
{
  _gst_plugin_register_static (&plugin_desc);
}

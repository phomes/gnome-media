/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2006-2008 Lennart Poettering
 * Copyright (C) 2008 Sjoerd Simons <sjoerd@luon.net>
 * Copyright (C) 2008 William Jon McCann
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <glib.h>
#include <glib/gi18n.h>

#include <pulse/pulseaudio.h>
#include <pulse/glib-mainloop.h>
#include <pulse/ext-stream-restore.h>

#include "gvc-mixer-control.h"
#include "gvc-mixer-sink.h"
#include "gvc-mixer-sink-input.h"

#define GVC_MIXER_CONTROL_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GVC_TYPE_MIXER_CONTROL, GvcMixerControlPrivate))

struct GvcMixerControlPrivate
{
        pa_glib_mainloop *pa_mainloop;
        pa_mainloop_api  *pa_api;
        pa_context       *pa_context;
        int               n_outstanding;

        char             *default_sink_name;
        guint             default_sink_index;
        char             *default_source_name;
        guint             default_source_index;

        GvcMixerStream   *event_sink_input;

        GHashTable       *sinks; /* fixed outputs */
        GHashTable       *sources; /* fixed inputs */
        GHashTable       *sink_inputs; /* routable output streams */
        GHashTable       *source_outputs; /* routable input streams */
        GHashTable       *clients;
};

enum {
        READY,
        STREAM_ADDED,
        STREAM_REMOVED,
        LAST_SIGNAL
};

static guint signals [LAST_SIGNAL] = { 0, };

static void     gvc_mixer_control_class_init (GvcMixerControlClass *klass);
static void     gvc_mixer_control_init       (GvcMixerControl      *mixer_control);
static void     gvc_mixer_control_finalize   (GObject              *object);

G_DEFINE_TYPE (GvcMixerControl, gvc_mixer_control, G_TYPE_OBJECT)

GvcMixerStream *
gvc_mixer_control_get_event_sink_input (GvcMixerControl *control)
{
        g_return_val_if_fail (GVC_IS_MIXER_CONTROL (control), NULL);

        return control->priv->event_sink_input;
}

GvcMixerStream *
gvc_mixer_control_get_default_sink (GvcMixerControl *control)
{
        GvcMixerStream *stream;

        g_return_val_if_fail (GVC_IS_MIXER_CONTROL (control), NULL);

        stream = g_hash_table_lookup (control->priv->sinks,
                                      GUINT_TO_POINTER (control->priv->default_sink_index));
        return stream;
}

static void
listify_hash_values_hfunc (gpointer key,
                           gpointer value,
                           gpointer user_data)
{
        GSList **list = user_data;

        *list = g_slist_prepend (*list, value);
}

static int
gvc_stream_collate (GvcMixerStream *a,
                    GvcMixerStream *b)
{
        const char *namea;
        const char *nameb;

        g_return_val_if_fail (a == NULL || GVC_IS_MIXER_STREAM (a), 0);
        g_return_val_if_fail (b == NULL || GVC_IS_MIXER_STREAM (b), 0);

        namea = gvc_mixer_stream_get_name (a);
        nameb = gvc_mixer_stream_get_name (b);

        return g_utf8_collate (namea, nameb);
}

GSList *
gvc_mixer_control_get_sinks (GvcMixerControl *control)
{
        GSList *retval;

        g_return_val_if_fail (GVC_IS_MIXER_CONTROL (control), NULL);

        retval = NULL;
        g_hash_table_foreach (control->priv->sinks,
                              listify_hash_values_hfunc,
                              &retval);
        return g_slist_sort (retval, (GCompareFunc) gvc_stream_collate);
}

GSList *
gvc_mixer_control_get_sink_inputs (GvcMixerControl *control)
{
        GSList *retval;

        g_return_val_if_fail (GVC_IS_MIXER_CONTROL (control), NULL);

        retval = NULL;
        g_hash_table_foreach (control->priv->sink_inputs,
                              listify_hash_values_hfunc,
                              &retval);
        return g_slist_sort (retval, (GCompareFunc) gvc_stream_collate);
}

static void
dec_outstanding (GvcMixerControl *control)
{
        if (control->priv->n_outstanding <= 0) {
                return;
        }

        if (--control->priv->n_outstanding <= 0) {
                g_signal_emit (G_OBJECT (control), signals[READY], 0);
        }
}

gboolean
gvc_mixer_control_is_ready (GvcMixerControl *control)
{
        g_return_val_if_fail (GVC_IS_MIXER_CONTROL (control), FALSE);

        return (control->priv->n_outstanding == 0);
}

static void
update_server (GvcMixerControl      *control,
               const pa_server_info *info)
{
        if (info->default_source_name != NULL) {
                g_debug ("Default source: %s", info->default_source_name);
                g_free (control->priv->default_source_name);
                control->priv->default_source_name = g_strdup (info->default_source_name);
                /* FIXME: iterate over existing sources and update default flag */
        }
        if (info->default_sink_name != NULL) {
                g_debug ("Default sink: %s", info->default_sink_name);
                g_free (control->priv->default_sink_name);
                control->priv->default_sink_name = g_strdup (info->default_sink_name);
                /* FIXME: iterate over existing sinks and update default flag */
        }
}

static void
update_sink (GvcMixerControl    *control,
             const pa_sink_info *info)
{
        GvcMixerStream *stream;
        gboolean        is_new;
        gboolean        is_default;
        pa_volume_t     avg_volume;
#if 0
        g_debug ("Updating sink: index=%u name='%s' description='%s'",
                 info->index,
                 info->name,
                 info->description);
#endif
        is_new = FALSE;
        is_default = FALSE;

        stream = g_hash_table_lookup (control->priv->sinks,
                                      GUINT_TO_POINTER (info->index));
        if (stream == NULL) {
                stream = gvc_mixer_sink_new (control->priv->pa_context,
                                             info->index,
                                             info->channel_map.channels);
                g_hash_table_insert (control->priv->sinks,
                                     GUINT_TO_POINTER (info->index),
                                     stream);
                is_new = TRUE;
        }

        if (control->priv->default_sink_name != NULL
            && info->name != NULL
            && strcmp (control->priv->default_sink_name, info->name) == 0) {
                is_default = TRUE;
        }
        avg_volume = pa_cvolume_avg (&info->volume);

        gvc_mixer_stream_set_name (stream, info->name);
        gvc_mixer_stream_set_icon_name (stream, "audio-card");
        gvc_mixer_stream_set_volume (stream, (guint)avg_volume);
        gvc_mixer_stream_set_is_muted (stream, info->mute);
        gvc_mixer_stream_set_is_default (stream, is_default);

        //w->type = info.flags & PA_SINK_HARDWARE ? SINK_HARDWARE : SINK_VIRTUAL;

        if (is_new) {
                g_signal_emit (G_OBJECT (control), signals[STREAM_ADDED], 0, info->index);
        }

        if (is_default) {
                control->priv->default_sink_index = info->index;
        }
}

static void
update_source (GvcMixerControl      *control,
               const pa_source_info *info)
{
        g_debug ("Updating source: index=%u name='%s' description='%s'",
                 info->index,
                 info->name,
                 info->description);

}

static void
set_icon_name_from_proplist (GvcMixerStream *stream,
                             pa_proplist    *l,
                             const char     *default_icon_name)
{
        const char *t;

        if ((t = pa_proplist_gets (l, PA_PROP_MEDIA_ICON_NAME))) {
                goto finish;
        }

        if ((t = pa_proplist_gets (l, PA_PROP_WINDOW_ICON_NAME))) {
                goto finish;
        }

        if ((t = pa_proplist_gets (l, PA_PROP_APPLICATION_ICON_NAME))) {
                goto finish;
        }

        if ((t = pa_proplist_gets (l, PA_PROP_MEDIA_ROLE))) {

                if (strcmp (t, "video") == 0 ||
                    strcmp (t, "phone") == 0) {
                        goto finish;
                }

                if (strcmp (t, "music") == 0) {
                        t = "audio";
                        goto finish;
                }

                if (strcmp (t, "game") == 0) {
                        t = "applications-games";
                        goto finish;
                }

                if (strcmp (t, "event") == 0) {
                        t = "dialog-information";
                        goto finish;
                }
        }

        t = default_icon_name;

 finish:
        gvc_mixer_stream_set_icon_name (stream, t);
}

static void
update_sink_input (GvcMixerControl          *control,
                   const pa_sink_input_info *info)
{
        GvcMixerStream *stream;
        gboolean        is_new;
        gboolean        is_default;
        pa_volume_t     avg_volume;

        g_debug ("Updating sink input: index=%u name='%s'",
                 info->index,
                 info->name);

        is_new = FALSE;
        is_default = FALSE;

        stream = g_hash_table_lookup (control->priv->sink_inputs,
                                      GUINT_TO_POINTER (info->index));
        if (stream == NULL) {
                stream = gvc_mixer_sink_input_new (control->priv->pa_context,
                                                   info->index,
                                                   info->channel_map.channels);
                g_hash_table_insert (control->priv->sink_inputs,
                                     GUINT_TO_POINTER (info->index),
                                     stream);
                is_new = TRUE;
        }

        avg_volume = pa_cvolume_avg (&info->volume);

        gvc_mixer_stream_set_name (stream, info->name);

        set_icon_name_from_proplist (stream, info->proplist, "applications-multimedia");
        gvc_mixer_stream_set_volume (stream, (guint)avg_volume);
        gvc_mixer_stream_set_is_muted (stream, info->mute);
        gvc_mixer_stream_set_is_default (stream, is_default);

        if (is_new) {
                g_signal_emit (G_OBJECT (control), signals[STREAM_ADDED], 0, info->index);
        }
}

static void
update_source_output (GvcMixerControl             *control,
                      const pa_source_output_info *info)
{
        g_debug ("Updating source output: index=%u name='%s'",
                 info->index,
                 info->name);
}

static void
update_client (GvcMixerControl      *control,
               const pa_client_info *info)
{
#if 0
        g_debug ("Updating client: index=%u name='%s'",
                 info->index,
                 info->name);
#endif
}

static void
_pa_context_get_sink_info_cb (pa_context         *context,
                              const pa_sink_info *i,
                              int                 eol,
                              void               *userdata)
{
        GvcMixerControl *control = GVC_MIXER_CONTROL (userdata);

        if (eol < 0) {
                if (pa_context_errno (context) == PA_ERR_NOENTITY) {
                        return;
                }

                g_warning ("Sink callback failure");
                return;
        }

        if (eol > 0) {
                dec_outstanding (control);
                return;
        }

        update_sink (control, i);
}


static void
_pa_context_get_source_info_cb (pa_context           *context,
                                const pa_source_info *i,
                                int                   eol,
                                void                 *userdata)
{
        GvcMixerControl *control = GVC_MIXER_CONTROL (userdata);

        if (eol < 0) {
                if (pa_context_errno (context) == PA_ERR_NOENTITY) {
                        return;
                }

                g_warning ("Source callback failure");
                return;
        }

        if (eol > 0) {
                dec_outstanding (control);
                return;
        }

        update_source (control, i);
}

static void
_pa_context_get_sink_input_info_cb (pa_context               *context,
                                    const pa_sink_input_info *i,
                                    int                       eol,
                                    void                     *userdata)
{
        GvcMixerControl *control = GVC_MIXER_CONTROL (userdata);

        if (eol < 0) {
                if (pa_context_errno (context) == PA_ERR_NOENTITY) {
                        return;
                }

                g_warning ("Sink input callback failure");
                return;
        }

        if (eol > 0) {
                dec_outstanding (control);
                return;
        }

        update_sink_input (control, i);
}

static void
_pa_context_get_source_output_info_cb (pa_context                  *context,
                                       const pa_source_output_info *i,
                                       int                          eol,
                                       void                        *userdata)
{
        GvcMixerControl *control = GVC_MIXER_CONTROL (userdata);

        if (eol < 0) {
                if (pa_context_errno (context) == PA_ERR_NOENTITY) {
                        return;
                }

                g_warning ("Source output callback failure");
                return;
        }

        if (eol > 0)  {
                dec_outstanding (control);
                return;
        }

        update_source_output (control, i);
}

static void
_pa_context_get_client_info_cb (pa_context           *context,
                                const pa_client_info *i,
                                int                   eol,
                                void                 *userdata)
{
        GvcMixerControl *control = GVC_MIXER_CONTROL (userdata);

        if (eol < 0) {
                if (pa_context_errno (context) == PA_ERR_NOENTITY) {
                        return;
                }

                g_warning ("Client callback failure");
                return;
        }

        if (eol > 0) {
                dec_outstanding (control);
                return;
        }

        update_client (control, i);
}

static void
_pa_context_get_server_info_cb (pa_context           *context,
                                const pa_server_info *i,
                                void                 *userdata)
{
        GvcMixerControl *control = GVC_MIXER_CONTROL (userdata);

        if (i == NULL) {
                g_warning ("Server info callback failure");
                return;
        }

        update_server (control, i);
        dec_outstanding (control);
}

static void
remove_event_role_stream (GvcMixerControl *control)
{
        g_debug ("Removing event role");
}

static void
update_event_role_stream (GvcMixerControl                  *control,
                          const pa_ext_stream_restore_info *info)
{
        GvcMixerStream *stream;
        gboolean        is_new;
        pa_volume_t     avg_volume;

        if (strcmp (info->name, "sink-input-by-media-role:event") != 0) {
                return;
        }

        g_debug ("Updating event role: name='%s' device='%s'",
                 info->name,
                 info->device);

        is_new = FALSE;

        if (control->priv->event_sink_input == NULL) {
                stream = gvc_mixer_sink_input_new (control->priv->pa_context,
                                                   0,
                                                   1);
                control->priv->event_sink_input = stream;
                is_new = TRUE;
        } else {
                stream = control->priv->event_sink_input;
        }

        avg_volume = pa_cvolume_avg (&info->volume);

        gvc_mixer_stream_set_name (stream, _("System Sounds"));
        //gvc_mixer_stream_set_name (stream, info->name);
        gvc_mixer_stream_set_icon_name (stream, "multimedia-volume-control");
        gvc_mixer_stream_set_volume (stream, (guint)avg_volume);
        gvc_mixer_stream_set_is_muted (stream, info->mute);
}

static void
_pa_ext_stream_restore_read_cb (pa_context                       *context,
                                const pa_ext_stream_restore_info *i,
                                int                               eol,
                                void                             *userdata)
{
        GvcMixerControl *control = GVC_MIXER_CONTROL (userdata);

        if (eol < 0) {
                g_debug ("Failed to initialized stream_restore extension: %s",
                         pa_strerror (pa_context_errno (context)));
                remove_event_role_stream (control);
                return;
        }

        if (eol > 0) {
                dec_outstanding (control);
                return;
        }

        update_event_role_stream (control, i);
}

static void
_pa_ext_stream_restore_subscribe_cb (pa_context *context,
                                     void       *userdata)
{
        GvcMixerControl *control = GVC_MIXER_CONTROL (userdata);
        pa_operation    *o;

        o = pa_ext_stream_restore_read (context,
                                        _pa_ext_stream_restore_read_cb,
                                        control);
        if (o == NULL) {
                g_warning ("pa_ext_stream_restore_read() failed");
                return;
        }

        pa_operation_unref (o);
}

static void
req_update_server_info (GvcMixerControl *control,
                        int              index)
{
        pa_operation *o;

        o = pa_context_get_server_info (control->priv->pa_context,
                                        _pa_context_get_server_info_cb,
                                        control);
        if (o == NULL) {
                g_warning ("pa_context_get_server_info() failed");
                return;
        }
        pa_operation_unref (o);
}

static void
req_update_client_info (GvcMixerControl *control,
                        int              index)
{
        pa_operation *o;

        if (index < 0) {
                o = pa_context_get_client_info_list (control->priv->pa_context,
                                                     _pa_context_get_client_info_cb,
                                                     control);
        } else {
                o = pa_context_get_client_info (control->priv->pa_context,
                                                index,
                                                _pa_context_get_client_info_cb,
                                                control);
        }

        if (o == NULL) {
                g_warning ("pa_context_client_info_list() failed");
                return;
        }
        pa_operation_unref (o);
}

static void
req_update_sink_info (GvcMixerControl *control,
                      int              index)
{
        pa_operation *o;

        if (index < 0) {
                o = pa_context_get_sink_info_list (control->priv->pa_context,
                                                   _pa_context_get_sink_info_cb,
                                                   control);
        } else {
                o = pa_context_get_sink_info_by_index (control->priv->pa_context,
                                                       index,
                                                       _pa_context_get_sink_info_cb,
                                                       control);
        }

        if (o == NULL) {
                g_warning ("pa_context_get_sink_info_list() failed");
                return;
        }
        pa_operation_unref (o);
}

static void
req_update_source_info (GvcMixerControl *control,
                        int              index)
{
        pa_operation *o;

        if (index < 0) {
                o = pa_context_get_source_info_list (control->priv->pa_context,
                                                     _pa_context_get_source_info_cb,
                                                     control);
        } else {
                o = pa_context_get_source_info_by_index(control->priv->pa_context,
                                                        index,
                                                        _pa_context_get_source_info_cb,
                                                        control);
        }

        if (o == NULL) {
                g_warning ("pa_context_get_source_info_list() failed");
                return;
        }
        pa_operation_unref (o);
}

static void
req_update_sink_input_info (GvcMixerControl *control,
                            int              index)
{
        pa_operation *o;

        if (index < 0) {
                o = pa_context_get_sink_input_info_list (control->priv->pa_context,
                                                         _pa_context_get_sink_input_info_cb,
                                                         control);
        } else {
                o = pa_context_get_sink_input_info (control->priv->pa_context,
                                                    index,
                                                    _pa_context_get_sink_input_info_cb,
                                                    control);
        }

        if (o == NULL) {
                g_warning ("pa_context_get_sink_input_info_list() failed");
                return;
        }
        pa_operation_unref (o);
}

static void
req_update_source_output_info (GvcMixerControl *control,
                               int              index)
{
        pa_operation *o;

        if (index < 0) {
                o = pa_context_get_source_output_info_list (control->priv->pa_context,
                                                            _pa_context_get_source_output_info_cb,
                                                            control);
        } else {
                o = pa_context_get_source_output_info (control->priv->pa_context,
                                                       index,
                                                       _pa_context_get_source_output_info_cb,
                                                       control);
        }

        if (o == NULL) {
                g_warning ("pa_context_get_source_output_info_list() failed");
                return;
        }
        pa_operation_unref (o);
}

static void
remove_client (GvcMixerControl *control,
               guint            index)
{
        g_debug ("Removing client: index=%u", index);
        /* FIXME: */
}

static void
remove_sink (GvcMixerControl *control,
             guint            index)
{
        g_debug ("Removing sink: index=%u", index);
        /* FIXME: */
}

static void
remove_source (GvcMixerControl *control,
               guint            index)
{
        g_debug ("Removing source: index=%u", index);
        /* FIXME: */
}

static void
remove_sink_input (GvcMixerControl *control,
                   guint            index)
{
        g_debug ("Removing source: index=%u", index);
        /* FIXME: */
}

static void
remove_source_output (GvcMixerControl *control,
                      guint            index)
{
        g_debug ("Removing source output: index=%u", index);
        /* FIXME: */
}

static void
_pa_context_subscribe_cb (pa_context                  *context,
                          pa_subscription_event_type_t t,
                          uint32_t                     index,
                          void                        *userdata)
{
        GvcMixerControl *control = GVC_MIXER_CONTROL (userdata);

        switch (t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK) {
        case PA_SUBSCRIPTION_EVENT_SINK:
                if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_REMOVE) {
                        remove_sink (control, index);
                } else {
                        req_update_sink_info (control, index);
                }
                break;

        case PA_SUBSCRIPTION_EVENT_SOURCE:
                if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_REMOVE) {
                        remove_source (control, index);
                } else {
                        req_update_source_info (control, index);
                }
                break;

        case PA_SUBSCRIPTION_EVENT_SINK_INPUT:
                if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_REMOVE) {
                        remove_sink_input (control, index);
                } else {
                        req_update_sink_input_info (control, index);
                }
                break;

        case PA_SUBSCRIPTION_EVENT_SOURCE_OUTPUT:
                if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_REMOVE) {
                        remove_source_output (control, index);
                } else {
                        req_update_source_output_info (control, index);
                }
                break;

        case PA_SUBSCRIPTION_EVENT_CLIENT:
                if ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_REMOVE) {
                        remove_client (control, index);
                } else {
                        req_update_client_info (control, index);
                }
                break;

        case PA_SUBSCRIPTION_EVENT_SERVER:
                req_update_server_info (control, index);
                break;
        }
}

static void
gvc_mixer_control_ready (GvcMixerControl *control)
{
        pa_operation *o;

        pa_context_set_subscribe_callback (control->priv->pa_context,
                                           _pa_context_subscribe_cb,
                                           control);
        o = pa_context_subscribe (control->priv->pa_context,
                                  (pa_subscription_mask_t)
                                  (PA_SUBSCRIPTION_MASK_SINK|
                                   PA_SUBSCRIPTION_MASK_SOURCE|
                                   PA_SUBSCRIPTION_MASK_SINK_INPUT|
                                   PA_SUBSCRIPTION_MASK_SOURCE_OUTPUT|
                                   PA_SUBSCRIPTION_MASK_CLIENT|
                                   PA_SUBSCRIPTION_MASK_SERVER),
                                  NULL,
                                  NULL);

        if (o == NULL) {
                g_warning ("pa_context_subscribe() failed");
                return;
        }
        pa_operation_unref (o);

        req_update_server_info (control, -1);
        req_update_client_info (control, -1);
        req_update_sink_info (control, -1);
        req_update_source_info (control, -1);
        req_update_sink_input_info (control, -1);
        req_update_source_output_info (control, -1);

        control->priv->n_outstanding = 6;

        /* This call is not always supported */
        o = pa_ext_stream_restore_read (control->priv->pa_context,
                                        _pa_ext_stream_restore_read_cb,
                                        control);
        if (o != NULL) {
                pa_operation_unref (o);
                control->priv->n_outstanding++;

                pa_ext_stream_restore_set_subscribe_cb (control->priv->pa_context,
                                                        _pa_ext_stream_restore_subscribe_cb,
                                                        control);


                o = pa_ext_stream_restore_subscribe (control->priv->pa_context,
                                                     1,
                                                     NULL,
                                                     NULL);
                if (o != NULL) {
                        pa_operation_unref (o);
                }

        } else {
                g_debug ("Failed to initialized stream_restore extension: %s",
                         pa_strerror (pa_context_errno (control->priv->pa_context)));
        }
}

static void
_pa_context_state_cb (pa_context *context,
                      void       *userdata)
{
        GvcMixerControl *control = GVC_MIXER_CONTROL (userdata);

        switch (pa_context_get_state (context)) {
        case PA_CONTEXT_UNCONNECTED:
        case PA_CONTEXT_CONNECTING:
        case PA_CONTEXT_AUTHORIZING:
        case PA_CONTEXT_SETTING_NAME:
                break;

        case PA_CONTEXT_READY:
                gvc_mixer_control_ready (control);
                break;

        case PA_CONTEXT_FAILED:
                g_warning ("Connection failed");
                break;

        case PA_CONTEXT_TERMINATED:
        default:
                /* FIXME: */
                break;
        }
}

gboolean
gvc_mixer_control_open (GvcMixerControl *control)
{
        int res;

        g_return_val_if_fail (GVC_IS_MIXER_CONTROL (control), FALSE);
        g_return_val_if_fail (control->priv->pa_context != NULL, FALSE);
        g_return_val_if_fail (pa_context_get_state (control->priv->pa_context) == PA_CONTEXT_UNCONNECTED, FALSE);

        pa_context_set_state_callback (control->priv->pa_context,
                                       _pa_context_state_cb,
                                       control);

        res = pa_context_connect (control->priv->pa_context, NULL, (pa_context_flags_t) 0, NULL);
        if (res < 0) {
                g_warning ("Failed to connect context: %s",
                           pa_strerror (pa_context_errno (control->priv->pa_context)));
        }

        return res;
}

gboolean
gvc_mixer_control_close (GvcMixerControl *control)
{
        g_return_val_if_fail (GVC_IS_MIXER_CONTROL (control), FALSE);
        g_return_val_if_fail (control->priv->pa_context != NULL, FALSE);

        pa_context_disconnect (control->priv->pa_context);
        return TRUE;
}

static void
gvc_mixer_control_dispose (GObject *object)
{
        GvcMixerControl *control = GVC_MIXER_CONTROL (object);

        if (control->priv->pa_context != NULL) {
                pa_context_unref (control->priv->pa_context);
                control->priv->pa_context = NULL;
        }

        if (control->priv->pa_mainloop != NULL) {
                pa_glib_mainloop_free (control->priv->pa_mainloop);
                control->priv->pa_mainloop = NULL;
        }

        G_OBJECT_CLASS (gvc_mixer_control_parent_class)->dispose (object);
}

static GObject *
gvc_mixer_control_constructor (GType                  type,
                               guint                  n_construct_properties,
                               GObjectConstructParam *construct_params)
{
        GObject         *object;
        GvcMixerControl *self;
        pa_proplist     *proplist;

        object = G_OBJECT_CLASS (gvc_mixer_control_parent_class)->constructor (type, n_construct_properties, construct_params);

        self = GVC_MIXER_CONTROL (object);

        /* FIXME: read these from an object property */
        proplist = pa_proplist_new ();
        pa_proplist_sets (proplist,
                          PA_PROP_APPLICATION_NAME,
                          _("GNOME Volume Control"));
        pa_proplist_sets (proplist,
                          PA_PROP_APPLICATION_ID,
                          "org.gnome.VolumeControl");
        pa_proplist_sets (proplist,
                          PA_PROP_APPLICATION_ICON_NAME,
                          "multimedia-volume-control");
        pa_proplist_sets (proplist,
                          PA_PROP_APPLICATION_VERSION,
                          PACKAGE_VERSION);

        self->priv->pa_context = pa_context_new_with_proplist (self->priv->pa_api, NULL, proplist);
        g_assert (self->priv->pa_context);
        pa_proplist_free (proplist);

        return object;
}

static void
gvc_mixer_control_class_init (GvcMixerControlClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);

        object_class->constructor = gvc_mixer_control_constructor;
        object_class->dispose = gvc_mixer_control_dispose;
        object_class->finalize = gvc_mixer_control_finalize;

        signals [READY] =
                g_signal_new ("ready",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GvcMixerControlClass, ready),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__VOID,
                              G_TYPE_NONE, 0);
        signals [STREAM_ADDED] =
                g_signal_new ("stream-added",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GvcMixerControlClass, stream_added),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__UINT,
                              G_TYPE_NONE, 1, G_TYPE_UINT);
        signals [STREAM_REMOVED] =
                g_signal_new ("stream-removed",
                              G_TYPE_FROM_CLASS (klass),
                              G_SIGNAL_RUN_LAST,
                              G_STRUCT_OFFSET (GvcMixerControlClass, stream_removed),
                              NULL, NULL,
                              g_cclosure_marshal_VOID__UINT,
                              G_TYPE_NONE, 1, G_TYPE_UINT);

        g_type_class_add_private (klass, sizeof (GvcMixerControlPrivate));
}

static void
gvc_mixer_control_init (GvcMixerControl *control)
{
        control->priv = GVC_MIXER_CONTROL_GET_PRIVATE (control);

        control->priv->pa_mainloop = pa_glib_mainloop_new (g_main_context_default ());
        g_assert (control->priv->pa_mainloop);

        control->priv->pa_api = pa_glib_mainloop_get_api (control->priv->pa_mainloop);
        g_assert (control->priv->pa_api);

        control->priv->sinks = g_hash_table_new_full (NULL, NULL, NULL, (GDestroyNotify)g_object_unref);
        control->priv->sources = g_hash_table_new_full (NULL, NULL, NULL, (GDestroyNotify)g_object_unref);
        control->priv->sink_inputs = g_hash_table_new_full (NULL, NULL, NULL, (GDestroyNotify)g_object_unref);
        control->priv->source_outputs = g_hash_table_new_full (NULL, NULL, NULL, (GDestroyNotify)g_object_unref);
        control->priv->clients = g_hash_table_new_full (NULL, NULL, NULL, (GDestroyNotify)g_object_unref);
}

static void
gvc_mixer_control_finalize (GObject *object)
{
        GvcMixerControl *mixer_control;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GVC_IS_MIXER_CONTROL (object));

        mixer_control = GVC_MIXER_CONTROL (object);

        g_return_if_fail (mixer_control->priv != NULL);
        G_OBJECT_CLASS (gvc_mixer_control_parent_class)->finalize (object);
}

GvcMixerControl *
gvc_mixer_control_new (void)
{
        GObject *control;
        control = g_object_new (GVC_TYPE_MIXER_CONTROL, NULL);
        return GVC_MIXER_CONTROL (control);
}
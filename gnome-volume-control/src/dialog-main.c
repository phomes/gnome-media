/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
 * 02111-1307, USA.
 */

#include "config.h"

#include <libintl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <glib/gi18n.h>
#include <glib/goption.h>
#include <gtk/gtk.h>

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>
#include <dbus/dbus-glib-lowlevel.h>

#include "gvc-mixer-dialog.h"

#define GVCA_DBUS_NAME "org.gnome.VolumeControl"

#define IS_STRING_EMPTY(x) ((x)==NULL||(x)[0]=='\0')

static gboolean show_version = FALSE;
static gboolean debug = FALSE;
static gchar* page = NULL;

static void
on_bus_name_lost (DBusGProxy *bus_proxy,
                  const char *name,
                  gpointer    data)
{
        g_warning ("Lost name on bus: %s, exiting", name);
        exit (1);
}

static gboolean
acquire_name_on_proxy (DBusGProxy *bus_proxy,
                       const char *name)
{
        GError     *error;
        guint       result;
        gboolean    res;
        gboolean    ret;

        ret = FALSE;

        if (bus_proxy == NULL) {
                goto out;
        }

        error = NULL;
        res = dbus_g_proxy_call (bus_proxy,
                                 "RequestName",
                                 &error,
                                 G_TYPE_STRING, name,
                                 G_TYPE_UINT, 0,
                                 G_TYPE_INVALID,
                                 G_TYPE_UINT, &result,
                                 G_TYPE_INVALID);
        if (! res) {
                if (error != NULL) {
                        g_warning ("Failed to acquire %s: %s", name, error->message);
                        g_error_free (error);
                } else {
                        g_warning ("Failed to acquire %s", name);
                }
                goto out;
        }

        if (result != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER) {
                if (error != NULL) {
                        g_warning ("Failed to acquire %s: %s", name, error->message);
                        g_error_free (error);
                } else {
                        g_warning ("Failed to acquire %s", name);
                }
                goto out;
        }

        /* register for name lost */
        dbus_g_proxy_add_signal (bus_proxy,
                                 "NameLost",
                                 G_TYPE_STRING,
                                 G_TYPE_INVALID);
        dbus_g_proxy_connect_signal (bus_proxy,
                                     "NameLost",
                                     G_CALLBACK (on_bus_name_lost),
                                     NULL,
                                     NULL);


        ret = TRUE;

 out:
        return ret;
}

static gboolean
acquire_name (void)
{
        DBusGProxy      *bus_proxy;
        GError          *error;
        DBusGConnection *connection;

        error = NULL;
        connection = dbus_g_bus_get (DBUS_BUS_SESSION, &error);
        if (connection == NULL) {
                g_warning ("Could not connect to session bus: %s",
                           error->message);
                exit (1);
        }

        bus_proxy = dbus_g_proxy_new_for_name (connection,
                                               DBUS_SERVICE_DBUS,
                                               DBUS_PATH_DBUS,
                                               DBUS_INTERFACE_DBUS);

        if (! acquire_name_on_proxy (bus_proxy, GVCA_DBUS_NAME) ) {
                g_warning ("Could not acquire name on session bus");
                exit (1);
        }

        g_object_unref (bus_proxy);

        return TRUE;
}

static void
on_dialog_response (GtkDialog *dialog,
                    guint      response_id,
                    gpointer   data)
{
        gtk_main_quit ();
}

static void
on_dialog_close (GtkDialog *dialog,
		 gpointer   data)
{
        gtk_main_quit ();
}

static void
on_control_ready (GvcMixerControl *control,
                  gpointer         data)
{
        GvcMixerDialog *dialog;

        dialog = gvc_mixer_dialog_new (control);
        g_signal_connect (dialog,
                          "response",
                          G_CALLBACK (on_dialog_response),
                          NULL);
        g_signal_connect (dialog,
                          "close",
                          G_CALLBACK (on_dialog_close),
                          NULL);

        if (page != NULL)
                gvc_mixer_dialog_set_page(dialog, page);

        gtk_widget_show (GTK_WIDGET (dialog));
}

int
main (int argc, char **argv)
{
        GError             *error;
        GvcMixerControl    *control;
        static GOptionEntry entries[] = {
                { "page", 'p', 0, G_OPTION_ARG_STRING, &page, N_("Startup page"), "playback|recording|effects|applications" },
                { "debug", 0, 0, G_OPTION_ARG_NONE, &debug, N_("Enable debugging code"), NULL },
                { "version", 0, 0, G_OPTION_ARG_NONE, &show_version, N_("Version of this application"), NULL },
                { NULL, 0, 0, 0, NULL, NULL, NULL }
        };

        bindtextdomain (GETTEXT_PACKAGE, LOCALE_DIR);
        bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
        textdomain (GETTEXT_PACKAGE);

        error = NULL;
        gtk_init_with_args (&argc, &argv,
                            (char *) _(" - GNOME Volume Control"),
                            entries, GETTEXT_PACKAGE,
                            &error);
        if (error != NULL) {
                g_warning ("%s", error->message);
                exit (1);
        }

        if (show_version) {
                g_print ("%s %s\n", argv [0], VERSION);
                exit (1);
        }

        acquire_name ();

        gtk_icon_theme_append_search_path (gtk_icon_theme_get_default (),
                                           ICON_DATA_DIR);

        control = gvc_mixer_control_new ();
        g_signal_connect (control,
                          "ready",
                          G_CALLBACK (on_control_ready),
                          control);
        gvc_mixer_control_open (control);

        /* FIXME: add timeout in case ready doesn't happen */

        gtk_main ();

        if (control != NULL) {
                g_object_unref (control);
        }

        return 0;
}

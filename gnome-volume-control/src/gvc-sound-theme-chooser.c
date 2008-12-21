/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2008 Bastien Nocera <hadess@hadess.net>
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
#include <gtk/gtk.h>
#include <canberra-gtk.h>

#include <gconf/gconf-client.h>

#include "gvc-sound-theme-chooser.h"
#include "sound-theme-file-utils.h"

#define GVC_SOUND_THEME_CHOOSER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), GVC_TYPE_SOUND_THEME_CHOOSER, GvcSoundThemeChooserPrivate))

struct GvcSoundThemeChooserPrivate
{
        GtkWidget *combo_box;
        GtkWidget *treeview;
        GtkWidget *theme_box;
        GtkWidget *selection_box;
        GtkWidget *click_feedback_button;
};

static void     gvc_sound_theme_chooser_class_init (GvcSoundThemeChooserClass *klass);
static void     gvc_sound_theme_chooser_init       (GvcSoundThemeChooser      *sound_theme_chooser);
static void     gvc_sound_theme_chooser_finalize   (GObject            *object);

G_DEFINE_TYPE (GvcSoundThemeChooser, gvc_sound_theme_chooser, GTK_TYPE_VBOX)

#define KEY_SOUNDS_DIR             "/desktop/gnome/sound"
#define EVENT_SOUNDS_KEY           KEY_SOUNDS_DIR "/event_sounds"
#define INPUT_SOUNDS_KEY           KEY_SOUNDS_DIR "/input_feedback_sounds"
#define SOUND_THEME_KEY            KEY_SOUNDS_DIR "/theme_name"
#define KEY_METACITY_DIR           "/apps/metacity/general"
#define AUDIO_BELL_KEY             KEY_METACITY_DIR "/audible_bell"

#define DEFAULT_ALERT_ID        "__default"
#define CUSTOM_THEME_NAME       "__custom"
#define NO_SOUNDS_THEME_NAME    "__no_sounds"

enum {
        THEME_DISPLAY_COL,
        THEME_IDENTIFIER_COL,
        THEME_PARENT_ID_COL,
        THEME_NUM_COLS
};

enum {
        ALERT_DISPLAY_COL,
        ALERT_IDENTIFIER_COL,
        ALERT_SOUND_TYPE_COL,
        ALERT_ACTIVE_COL,
        ALERT_NUM_COLS
};

enum {
        SOUND_TYPE_DEFAULT_FROM_THEME,
        SOUND_TYPE_BUILTIN,
        SOUND_TYPE_CUSTOM
};

static void
on_combobox_changed (GtkComboBox          *widget,
                     GvcSoundThemeChooser *chooser)
{
        GtkTreeIter   iter;
        GtkTreeModel *model;
        GConfClient  *client;
        char         *theme_name;

        if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (chooser->priv->combo_box), &iter) == FALSE) {
                return;
        }

        model = gtk_combo_box_get_model (GTK_COMBO_BOX (chooser->priv->combo_box));
        gtk_tree_model_get (model, &iter, THEME_IDENTIFIER_COL, &theme_name, -1);

        g_assert (theme_name != NULL);

        client = gconf_client_get_default ();

        /* special case for no sounds */
        if (strcmp (theme_name, NO_SOUNDS_THEME_NAME) == 0) {
                gconf_client_set_bool (client, EVENT_SOUNDS_KEY, FALSE, NULL);
                g_object_unref (client);
                return;
        } else {
                gconf_client_set_bool (client, EVENT_SOUNDS_KEY, TRUE, NULL);
        }

        gconf_client_set_string (client, SOUND_THEME_KEY, theme_name, NULL);
        g_object_unref (client);

        /* FIXME: reset alert model */
}

static char *
load_index_theme_name (const char *index,
                       char      **parent)
{
        GKeyFile *file;
        char *indexname = NULL;
        gboolean hidden;

        file = g_key_file_new ();
        if (g_key_file_load_from_file (file, index, G_KEY_FILE_KEEP_TRANSLATIONS, NULL) == FALSE) {
                g_key_file_free (file);
                return NULL;
        }
        /* Don't add hidden themes to the list */
        hidden = g_key_file_get_boolean (file, "Sound Theme", "Hidden", NULL);
        if (!hidden) {
                indexname = g_key_file_get_locale_string (file,
                                                          "Sound Theme",
                                                          "Name",
                                                          NULL,
                                                          NULL);

                /* Save the parent theme, if there's one */
                if (parent != NULL) {
                        *parent = g_key_file_get_string (file,
                                                         "Sound Theme",
                                                         "Inherits",
                                                         NULL);
                }
        }

        g_key_file_free (file);
        return indexname;
}

static void
sound_theme_in_dir (GHashTable *hash,
                    const char *dir)
{
        GDir *d;
        const char *name;

        d = g_dir_open (dir, 0, NULL);
        if (d == NULL) {
                return;
        }

        while ((name = g_dir_read_name (d)) != NULL) {
                char *dirname, *index, *indexname;

                /* Look for directories */
                dirname = g_build_filename (dir, name, NULL);
                if (g_file_test (dirname, G_FILE_TEST_IS_DIR) == FALSE) {
                        g_free (dirname);
                        continue;
                }

                /* Look for index files */
                index = g_build_filename (dirname, "index.theme", NULL);
                g_free (dirname);

                /* Check the name of the theme in the index.theme file */
                indexname = load_index_theme_name (index, NULL);
                g_free (index);
                if (indexname == NULL) {
                        continue;
                }

                g_hash_table_insert (hash, g_strdup (name), indexname);
        }

        g_dir_close (d);
}

static void
add_theme_to_store (const char   *key,
                    const char   *value,
                    GtkListStore *store)
{
        char *parent;

        parent = NULL;

        /* Get the parent, if we're checking the custom theme */
        if (strcmp (key, CUSTOM_THEME_NAME) == 0) {
                char *name, *path;

                path = custom_theme_dir_path ("index.theme");
                name = load_index_theme_name (path, &parent);
                g_free (name);
                g_free (path);
        }
        gtk_list_store_insert_with_values (store, NULL, G_MAXINT,
                                           THEME_DISPLAY_COL, value,
                                           THEME_IDENTIFIER_COL, key,
                                           THEME_PARENT_ID_COL, parent,
                                           -1);
        g_free (parent);
}

static void
set_combox_for_theme_name (GvcSoundThemeChooser *chooser,
                           const char           *name)
{
        GtkTreeIter   iter;
        GtkTreeModel *model;
        gboolean      found;

        g_debug ("setting theme %s", name ? name : "(null)");

        /* If the name is empty, use "freedesktop" */
        if (name == NULL || *name == '\0') {
                name = "freedesktop";
        }

        model = gtk_combo_box_get_model (GTK_COMBO_BOX (chooser->priv->combo_box));

        if (gtk_tree_model_get_iter_first (model, &iter) == FALSE) {
                return;
        }

        do {
                char *value;

                gtk_tree_model_get (model, &iter, THEME_IDENTIFIER_COL, &value, -1);
                found = (value != NULL && strcmp (value, name) == 0);
                g_free (value);

        } while (!found && gtk_tree_model_iter_next (model, &iter));

        /* When we can't find the theme we need to set, try to set the default
         * one "freedesktop" */
        if (found) {
                gtk_combo_box_set_active_iter (GTK_COMBO_BOX (chooser->priv->combo_box), &iter);
        } else if (strcmp (name, "freedesktop") != 0) {
                g_debug ("not found, falling back to fdo");
                set_combox_for_theme_name (chooser, "freedesktop");
        }
}

static void
set_input_feedback_enabled (GvcSoundThemeChooser *chooser,
                            gboolean              enabled)
{
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (chooser->priv->click_feedback_button),
                                      enabled);
}

static void
update_theme (GvcSoundThemeChooser *chooser)
{
        char        *theme_name;
        gboolean     events_enabled;
        gboolean     bell_enabled;
        GConfClient *client;
        gboolean     feedback_enabled;

        client = gconf_client_get_default ();

        bell_enabled = gconf_client_get_bool (client, AUDIO_BELL_KEY, NULL);
        //set_audible_bell_enabled (chooser, bell_enabled);

        feedback_enabled = gconf_client_get_bool (client, INPUT_SOUNDS_KEY, NULL);
        set_input_feedback_enabled (chooser, feedback_enabled);

        events_enabled = gconf_client_get_bool (client, EVENT_SOUNDS_KEY, NULL);
        if (events_enabled) {
                theme_name = gconf_client_get_string (client, SOUND_THEME_KEY, NULL);
        } else {
                theme_name = g_strdup (NO_SOUNDS_THEME_NAME);
        }

        gtk_widget_set_sensitive (chooser->priv->selection_box, events_enabled);

        set_combox_for_theme_name (chooser, theme_name);

        /* FIXME: update alerts */

        g_free (theme_name);

        g_object_unref (client);
}

static void
setup_theme_selector (GvcSoundThemeChooser *chooser)
{
        GHashTable           *hash;
        GtkListStore         *store;
        GtkCellRenderer      *renderer;
        const char * const   *data_dirs;
        const char           *data_dir;
        char                 *dir;
        guint                 i;

        /* Add the theme names and their display name to a hash table,
         * makes it easy to avoid duplicate themes */
        hash = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);

        data_dirs = g_get_system_data_dirs ();
        for (i = 0; data_dirs[i] != NULL; i++) {
                dir = g_build_filename (data_dirs[i], "sounds", NULL);
                sound_theme_in_dir (hash, dir);
                g_free (dir);
        }

        data_dir = g_get_user_data_dir ();
        dir = g_build_filename (data_dir, "sounds", NULL);
        sound_theme_in_dir (hash, dir);
        g_free (dir);

        /* If there isn't at least one theme, make everything
         * insensitive, LAME! */
        if (g_hash_table_size (hash) == 0) {
                gtk_widget_set_sensitive (GTK_WIDGET (chooser), FALSE);
                g_warning ("Bad setup, install the freedesktop sound theme");
                g_hash_table_destroy (hash);
                return;
        }

        /* Setup the tree model, 3 columns:
         * - internal theme name/directory
         * - display theme name
         * - the internal id for the parent theme, used for the custom theme */
        store = gtk_list_store_new (THEME_NUM_COLS,
                                    G_TYPE_STRING,
                                    G_TYPE_STRING,
                                    G_TYPE_STRING);

        /* Add the themes to a combobox */
        gtk_list_store_insert_with_values (store,
                                           NULL,
                                           G_MAXINT,
                                           THEME_DISPLAY_COL, _("No sounds"),
                                           THEME_IDENTIFIER_COL, "__no_sounds",
                                           THEME_PARENT_ID_COL, NULL,
                                           -1);
        g_hash_table_foreach (hash, (GHFunc) add_theme_to_store, store);
        g_hash_table_destroy (hash);

        /* Set the display */
        gtk_combo_box_set_model (GTK_COMBO_BOX (chooser->priv->combo_box),
                                 GTK_TREE_MODEL (store));

        renderer = gtk_cell_renderer_text_new ();
        gtk_cell_layout_pack_start (GTK_CELL_LAYOUT (chooser->priv->combo_box),
                                    renderer,
                                    TRUE);
        gtk_cell_layout_set_attributes (GTK_CELL_LAYOUT (chooser->priv->combo_box),
                                        renderer,
                                        "text", THEME_DISPLAY_COL,
                                        NULL);

        g_signal_connect (chooser->priv->combo_box,
                          "changed",
                          G_CALLBACK (on_combobox_changed),
                          chooser);
}

static void
on_alert_toggled (GtkCellRendererToggle *renderer,
                  char                  *path_str,
                  GvcSoundThemeChooser  *chooser)
{
        GtkTreeModel *model;
        GtkTreeIter   iter;
        GtkTreePath  *path;
        gboolean      toggled;
        char         *id;

        model = gtk_tree_view_get_model (GTK_TREE_VIEW (chooser->priv->treeview));

        path = gtk_tree_path_new_from_string (path_str);
        gtk_tree_model_get_iter (model, &iter, path);
        gtk_tree_path_free (path);

        id = NULL;
        gtk_tree_model_get (model, &iter,
                            ALERT_IDENTIFIER_COL, &id,
                            ALERT_ACTIVE_COL, &toggled,
                            -1);

        toggled ^= 1;
        if (toggled) {
                g_debug ("Default input selected: %s", id);
        }

        g_free (id);
}

static void
play_preview_for_path (GvcSoundThemeChooser *chooser,
                       GtkTreePath          *path)
{
        GtkTreeModel *model;
        GtkTreeIter   iter;
        char         *id;
        char         *filename;
        ca_context   *ctx;

        model = gtk_tree_view_get_model (GTK_TREE_VIEW (chooser->priv->treeview));
        if (gtk_tree_model_get_iter (model, &iter, path) == FALSE) {
                return;
        }

        id = NULL;
        gtk_tree_model_get (model, &iter,
                            ALERT_IDENTIFIER_COL, &id,
                            -1);
        if (id == NULL) {
                return;
        }

        filename = NULL;

        g_debug ("playing: %s", id);
        ctx = ca_gtk_context_get ();
        if (strcmp (id, DEFAULT_ALERT_ID) == 0) {
                ca_gtk_play_for_widget (GTK_WIDGET (chooser), 0,
                                        CA_PROP_APPLICATION_NAME, _("Sound Preferences"),
                                        CA_PROP_EVENT_ID, "bell-window-system",
                                        CA_PROP_EVENT_DESCRIPTION, _("Testing event sound"),
                                        CA_PROP_CANBERRA_CACHE_CONTROL, "never",
#ifdef CA_PROP_CANBERRA_ENABLE
                                        CA_PROP_CANBERRA_ENABLE, "1",
#endif
                                        NULL);
        } else {
                ca_gtk_play_for_widget (GTK_WIDGET (chooser), 0,
                                        CA_PROP_APPLICATION_NAME, _("Sound Preferences"),
                                        CA_PROP_MEDIA_FILENAME, filename,
                                        CA_PROP_EVENT_DESCRIPTION, _("Testing event sound"),
                                        CA_PROP_CANBERRA_CACHE_CONTROL, "never",
#ifdef CA_PROP_CANBERRA_ENABLE
                                        CA_PROP_CANBERRA_ENABLE, "1",
#endif
                                        NULL);

        }

        g_free (id);
}

static void
on_treeview_row_activated (GtkTreeView          *treeview,
                           GtkTreePath          *path,
                           GtkTreeViewColumn    *column,
                           GvcSoundThemeChooser *chooser)
{
        play_preview_for_path (chooser, path);
}

static void
on_treeview_selection_changed (GtkTreeSelection     *selection,
                               GvcSoundThemeChooser *chooser)
{
        GList        *paths;
        GtkTreeModel *model;
        GtkTreePath  *path;

        if (chooser->priv->treeview == NULL) {
                return;
        }

        model = gtk_tree_view_get_model (GTK_TREE_VIEW (chooser->priv->treeview));

        paths = gtk_tree_selection_get_selected_rows (selection, &model);

        path = paths->data;

        play_preview_for_path (chooser, path);

        g_list_foreach (paths, (GFunc)gtk_tree_path_free, NULL);
        g_list_free (paths);
}

static GtkWidget *
create_alert_treeview (GvcSoundThemeChooser *chooser)
{
        GHashTable           *hash;
        GtkListStore         *store;
        GtkWidget            *treeview;
        GtkCellRenderer      *renderer;
        GtkTreeViewColumn    *column;
        GtkTreeSelection     *selection;

        treeview = gtk_tree_view_new ();
        g_signal_connect (treeview,
                          "row-activated",
                          G_CALLBACK (on_treeview_row_activated),
                          chooser);

        selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
        gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
        g_signal_connect (selection,
                          "changed",
                          G_CALLBACK (on_treeview_selection_changed),
                          chooser);

        /* Add the theme names and their display name to a hash table,
         * makes it easy to avoid duplicate themes */
        hash = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);


        /* FIXME: need to read this from a set of deskop files */


        /* Setup the tree model, 3 columns:
         * - display name
         * - sound id
         * - sound type
         */
        store = gtk_list_store_new (ALERT_NUM_COLS,
                                    G_TYPE_STRING,
                                    G_TYPE_STRING,
                                    G_TYPE_STRING,
                                    G_TYPE_BOOLEAN);

        gtk_list_store_insert_with_values (store,
                                           NULL,
                                           G_MAXINT,
                                           ALERT_IDENTIFIER_COL, DEFAULT_ALERT_ID,
                                           ALERT_DISPLAY_COL, _("Default"),
                                           ALERT_SOUND_TYPE_COL, _("From theme"),
                                           ALERT_ACTIVE_COL, TRUE,
                                           -1);

        g_hash_table_foreach (hash, (GHFunc) add_theme_to_store, store);
        g_hash_table_destroy (hash);

        gtk_tree_view_set_model (GTK_TREE_VIEW (treeview),
                                 GTK_TREE_MODEL (store));

        renderer = gtk_cell_renderer_toggle_new ();
        gtk_cell_renderer_toggle_set_radio (GTK_CELL_RENDERER_TOGGLE (renderer),
                                            TRUE);
        column = gtk_tree_view_column_new_with_attributes (NULL,
                                                           renderer,
                                                           "active", ALERT_ACTIVE_COL,
                                                           NULL);
        gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);
        g_signal_connect (renderer,
                          "toggled",
                          G_CALLBACK (on_alert_toggled),
                          chooser);

        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Name"),
                                                           renderer,
                                                           "text", ALERT_DISPLAY_COL,
                                                           NULL);
        gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

        renderer = gtk_cell_renderer_text_new ();
        column = gtk_tree_view_column_new_with_attributes (_("Type"),
                                                           renderer,
                                                           "text", ALERT_SOUND_TYPE_COL,
                                                           NULL);
        gtk_tree_view_append_column (GTK_TREE_VIEW (treeview), column);

        return treeview;
}

static GObject *
gvc_sound_theme_chooser_constructor (GType                  type,
                                     guint                  n_construct_properties,
                                     GObjectConstructParam *construct_params)
{
        GObject              *object;
        GvcSoundThemeChooser *self;

        object = G_OBJECT_CLASS (gvc_sound_theme_chooser_parent_class)->constructor (type, n_construct_properties, construct_params);

        self = GVC_SOUND_THEME_CHOOSER (object);

        setup_theme_selector (self);

        update_theme (self);

        return object;
}

static void
gvc_sound_theme_chooser_class_init (GvcSoundThemeChooserClass *klass)
{
        GObjectClass   *object_class = G_OBJECT_CLASS (klass);

        object_class->constructor = gvc_sound_theme_chooser_constructor;
        object_class->finalize = gvc_sound_theme_chooser_finalize;

        g_type_class_add_private (klass, sizeof (GvcSoundThemeChooserPrivate));
}

static void
on_click_feedback_toggled (GtkToggleButton      *button,
                           GvcSoundThemeChooser *chooser)
{
        GConfClient *client;
        gboolean     enabled;

        enabled = gtk_toggle_button_get_active (button);

        client = gconf_client_get_default ();
        gconf_client_set_bool (client, INPUT_SOUNDS_KEY, enabled, NULL);
        g_object_unref (client);
}

static void
on_key_changed (GConfClient          *client,
                guint                 cnxn_id,
                GConfEntry           *entry,
                GvcSoundThemeChooser *chooser)
{
        const char *key;
        GConfValue *value;

        key = gconf_entry_get_key (entry);

        if (! g_str_has_prefix (key, KEY_SOUNDS_DIR)
            && ! g_str_has_prefix (key, KEY_METACITY_DIR)) {
                return;
        }

        value = gconf_entry_get_value (entry);
        if (strcmp (key, EVENT_SOUNDS_KEY) == 0) {
                update_theme (chooser);
        } else if (strcmp (key, SOUND_THEME_KEY) == 0) {
                update_theme (chooser);
        } else if (strcmp (key, INPUT_SOUNDS_KEY) == 0) {
                update_theme (chooser);
        } else if (strcmp (key, AUDIO_BELL_KEY) == 0) {
                update_theme (chooser);
        }
}

static void
constrain_list_size (GtkWidget      *widget,
                     GtkRequisition *requisition,
                     GtkWidget      *to_size)
{
        GtkRequisition req;
        int            max_height;

        /* constrain height to be the tree height up to a max */
        max_height = (gdk_screen_get_height (gtk_widget_get_screen (widget))) / 4;

        gtk_widget_size_request (to_size, &req);

        requisition->height = MIN (req.height, max_height);
}

static void
setup_list_size_constraint (GtkWidget *widget,
                            GtkWidget *to_size)
{
        g_signal_connect (widget,
                          "size-request",
                          G_CALLBACK (constrain_list_size),
                          to_size);
}

static void
gvc_sound_theme_chooser_init (GvcSoundThemeChooser *chooser)
{
        GtkWidget   *box;
        GtkWidget   *label;
        GtkWidget   *scrolled_window;
        GConfClient *client;

        chooser->priv = GVC_SOUND_THEME_CHOOSER_GET_PRIVATE (chooser);

        chooser->priv->theme_box = gtk_hbox_new (FALSE, 6);
        gtk_box_pack_start (GTK_BOX (chooser),
                            chooser->priv->theme_box, FALSE, FALSE, 0);

        label = gtk_label_new (_("Sound Theme:"));
        gtk_box_pack_start (GTK_BOX (chooser->priv->theme_box), label, FALSE, FALSE, 6);
        chooser->priv->combo_box = gtk_combo_box_new ();
        gtk_box_pack_start (GTK_BOX (chooser->priv->theme_box), chooser->priv->combo_box, FALSE, FALSE, 0);


        client = gconf_client_get_default ();

        chooser->priv->selection_box = box = gtk_frame_new (_("Choose an alert sound:"));
        gtk_frame_set_shadow_type (GTK_FRAME (box), GTK_SHADOW_NONE);

        gtk_box_pack_start (GTK_BOX (chooser), box, TRUE, TRUE, 0);

        chooser->priv->treeview = create_alert_treeview (chooser);

        scrolled_window = gtk_scrolled_window_new (NULL, NULL);
        setup_list_size_constraint (scrolled_window, chooser->priv->treeview);

        gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
                                        GTK_POLICY_NEVER,
                                        GTK_POLICY_AUTOMATIC);
        gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolled_window),
                                             GTK_SHADOW_IN);
        gtk_container_add (GTK_CONTAINER (scrolled_window), chooser->priv->treeview);
        gtk_container_add (GTK_CONTAINER (box), scrolled_window);

        chooser->priv->click_feedback_button = gtk_check_button_new_with_mnemonic (_("Enable window and button sounds"));
        gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (chooser->priv->click_feedback_button),
                                      gconf_client_get_bool (client, INPUT_SOUNDS_KEY, NULL));
        gtk_box_pack_start (GTK_BOX (chooser),
                            chooser->priv->click_feedback_button,
                            FALSE, FALSE, 0);
        g_signal_connect (chooser->priv->click_feedback_button,
                          "toggled",
                          G_CALLBACK (on_click_feedback_toggled),
                          chooser);


        gconf_client_add_dir (client, KEY_SOUNDS_DIR,
                              GCONF_CLIENT_PRELOAD_ONELEVEL,
                              NULL);
        gconf_client_notify_add (client,
                                 KEY_SOUNDS_DIR,
                                 (GConfClientNotifyFunc)on_key_changed,
                                 chooser, NULL, NULL);
        gconf_client_add_dir (client, KEY_METACITY_DIR,
                              GCONF_CLIENT_PRELOAD_ONELEVEL,
                              NULL);
        gconf_client_notify_add (client,
                                 KEY_METACITY_DIR,
                                 (GConfClientNotifyFunc)on_key_changed,
                                 chooser, NULL, NULL);

        g_object_unref (client);

        /* FIXME: should accept drag and drop themes.  should also
           add an "Add Theme..." item to the theme combobox */
}

static void
gvc_sound_theme_chooser_finalize (GObject *object)
{
        GvcSoundThemeChooser *sound_theme_chooser;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GVC_IS_SOUND_THEME_CHOOSER (object));

        sound_theme_chooser = GVC_SOUND_THEME_CHOOSER (object);

        g_return_if_fail (sound_theme_chooser->priv != NULL);

        G_OBJECT_CLASS (gvc_sound_theme_chooser_parent_class)->finalize (object);
}

GtkWidget *
gvc_sound_theme_chooser_new (void)
{
        GObject *chooser;
        chooser = g_object_new (GVC_TYPE_SOUND_THEME_CHOOSER,
                                "spacing", 6,
                                NULL);
        return GTK_WIDGET (chooser);
}

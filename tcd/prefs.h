#ifndef PROPERTIES_H__
#define PROPERTIES_H__

typedef enum
{
    DoNothing=0, 
    StopPlaying, 
    StartPlaying, 
    OpenTray, 
    CloseTray
} TCDAction;

typedef struct
{
    gint key;
    gboolean ctrl, alt, shift;
} Shortcut;

typedef struct
{
    gchar *cddev;
    gint time_display;
    gboolean handle;
    gchar *mixer_cmd;
    gboolean tooltip;
    gchar *trackfont;

    gint trackcolor_r, trackcolor_g, trackcolor_b;

    TCDAction exit_action, start_action;
    gboolean close_tray_on_start;

    int x,y,h,w;

    Shortcut quit, play, stop, tracked, mixer;
    Shortcut eject, back, forward;

    gchar *cddb_server;
    guint cddb_port;

    gboolean only_use_trkind;
} tcd_prefs;

void load_prefs( tcd_prefs *prop );
void save_prefs( tcd_prefs *prop );
void prefs_cb( GtkWidget *widget, void *data );
void changed_cb(GtkWidget *widget, void *data);

#endif

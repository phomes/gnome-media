#ifndef PROPERTIES_H__
#define PROPERTIES_H__

#define DEFAULT_FONT _("-adobe-helvetica-medium-r-normal-*-*-120-*-*-p-*-*-*")
#define EMERGENCY_FONT _("-*-fixed-*-*-*-*-*-*-*-*-*-*-*-*")

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
    gboolean cddb_http;
    gchar *cddb_httpproxy;
    gboolean cddb_httpproxy_need_auth;
    gchar *cddb_httpproxy_auth_name;
    gchar *cddb_httpproxy_auth_passwd;

    gboolean use_socks;
    gchar *socks_server;

    gboolean only_use_trkind;
    gboolean squared_volume;
} tcd_prefs;

void load_prefs( tcd_prefs *prop );
void save_prefs( tcd_prefs *prop );
void prefs_cb( GtkWidget *widget, void *data );
void changed_cb(GtkWidget *widget, void *data);

#endif

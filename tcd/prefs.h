#ifndef PROPERTIES_H__
#define PROPERTIES_H__

typedef struct
{
	gchar *cddev;
	gchar *cddb, *remote_path, *proxy_server;
	gint  cddbport, proxy_port, time_display;
	gboolean handle;
	gboolean tooltip;
	gboolean use_http, use_proxy;
	gchar *trackfont;
	gchar *statusfont;
	gchar *trackcolor;
	gchar *statuscolor;
} tcd_prefs;

void load_prefs( tcd_prefs *prop );
void save_prefs( tcd_prefs *prop );
void prefs_cb( GtkWidget *widget, void *data );

#endif

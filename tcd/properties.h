#ifndef PROPERTIES_H__
#define PROPERTIES_H__

typedef struct
{
	gchar *cddev;
	gchar *cddb, *remote_path, *proxy_server;
	gint  cddbport, proxy_port;
	gboolean handle;
	gboolean tooltip;
	gboolean use_http, use_proxy;
	gchar *trackfont;
	gchar *statusfont;
	gchar *trackcolor;
	gchar *statuscolor;
} tcd_properties;

void load_properties( tcd_properties *prop );
void save_properties( tcd_properties *prop );

#endif

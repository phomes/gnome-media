#ifndef PROPERTIES_H__
#define PROPERTIES_H__

typedef struct
{
	gchar *cddev;
	gchar *cddb;
	gchar *iconset;
	gint  cddbport;
	gboolean handle;
	gboolean tooltips;
} tcd_properties;

void load_properties( tcd_properties *prop );
void save_properties( tcd_properties *prop );

#endif

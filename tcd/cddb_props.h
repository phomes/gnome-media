#ifndef CDDB_PROPS_H
#define CDDB_PROPS_H

typedef struct
{
    GtkWidget *window;
    FILE *fp;
} EditWindow;

GtkWidget *create_cddb_page(void);

#endif

#ifndef GTCD_PUBLIC_H
#define GTCD_PUBLIC_H

#include <gnome.h>
#include <sys/types.h>
#include <linux/cdrom.h>

#include "linux-cdrom.h"
#include "prefs.h"

void make_goto_menu();
void create_warning(char *message_text, char *type);
void draw_status(void);
void gcddb(GtkWidget *widget, gpointer *data);
void preferences(GtkWidget *widget, void *data);
void setup_colors(void);
void setup_fonts(void);

extern int titlelabel_f;
extern cd_struct cd;
extern tcd_prefs prefs;
extern GtkTooltips *tooltips;

#endif

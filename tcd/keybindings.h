#ifndef GTCD_KEYBINDINGS_H
#define GTCD_KEYBINDINGS_H

typedef struct
{
	char key;
	char *desc;
	char *signal;
	GtkWidget *widget;
} KeyBinding;

void add_key_binding(GtkWidget *widget, char *signal, char *desc, char key);

#endif

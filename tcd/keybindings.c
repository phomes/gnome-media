#include <gnome.h>

#include "keybindings.h"
#include "gtcd_public.h"

GList *keys=NULL;

void add_key_binding(GtkWidget *widget, char *signal, char *desc, char key)
{
	KeyBinding *kb;

	kb = g_new(KeyBinding, 1);

	kb->widget = widget;
	kb->signal = signal;
	kb->desc = desc;
	kb->key = key;

	gtk_widget_add_accelerator(widget, signal, accel, key, 0, GTK_ACCEL_VISIBLE);

	keys = g_list_append(keys, kb);
}

#include <gnome.h>

#include "keybindings.h"
#include "gtcd_public.h"
#include "prefs.h"

GList *keys=NULL;

void add_key_binding(GtkWidget *widget, char *signal, char *desc, Shortcut *key)
{
	KeyBinding *kb;

	kb = g_new(KeyBinding, 1);

	kb->widget = widget;
	kb->signal = signal;
	kb->desc = desc;
	kb->key = key;

	gtk_widget_add_accelerator(widget, signal, accel, key->key, 0, GTK_ACCEL_VISIBLE);

	keys = g_list_append(keys, kb);
}

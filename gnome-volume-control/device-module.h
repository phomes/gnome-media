#ifndef DEVICE_MODULE_H
#define DEVICE_MODULE_H

#include <glib.h>

typedef GList * (* GetMixersFunc) (GError **error);

typedef void (*testf) (void);

#endif

#include "alsa-mixer.h"

GList *
get_mixers (GError **error)
{
  GList *list = NULL;
  g_list_append (list, alsa_mixer_new (0, error));

  return list;
}

#include "alsa-mixer.h"

GList *
get_mixers (GError **error)
{
  GList *list = NULL;
  GObject *mixer;

  mixer = alsa_mixer_new (0, error);
  if (!*error) {
    list = g_list_append (list, mixer);
  }

  return list;
}

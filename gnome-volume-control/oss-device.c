#include "oss-mixer.h"

GList *
get_mixers (GError **error)
{
  GList *list = NULL;
  printf ("this is crap!\n");
  g_list_append (list, oss_mixer_new (0, error));

  return list;
}

#include <stdio.h>
#include <orb/orbit.h>
#include <libgnorba/gnorba.h>
#include "cdaudio.h"

GNOME_CDAudio cdaudio_client;

int main(int argc, char *argv[])
{
    CORBA_Environment ev;
    CORBA_ORB orb;
    CORBA_long rv = 0, rv2;
    GNOME_CDAudio_Info *info;
    
    char buf[30];
    GoadServerList *servlist, *chosen = NULL;
    int i;

    CORBA_exception_init(&ev);
    orb = gnome_CORBA_init("CDAudio Client", "0.0", &argc, argv, 0, &ev);

    cdaudio_client = goad_server_activate_with_repo_id(0, "IDL:GNOME/CDAudio:1.0", GOAD_ACTIVATE_REMOTE, NULL);

    if(cdaudio_client == CORBA_OBJECT_NIL)
    {
	fprintf(stderr,"Cannot activate server\n");
	exit(1);
    }

    info = GNOME_CDAudio_init_cd(cdaudio_client, "/dev/cdrom", &rv, &ev);
    g_print("%s\n%d\n%d\n", info->path, info->fd, info->status);
//    GNOME_CDAudio_play_cd(cdaudio_client, 1, 2, &rv, &ev);
//    g_print("%ld\n", rv);
 
//    CORBA_Object_release(cdaudio_client, &ev);
//    CORBA_Object_release((CORBA_Object)orb, &ev);

    return 0;
}

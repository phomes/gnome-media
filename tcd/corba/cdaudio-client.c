#include <stdio.h>
#include <orb/orbit.h>
#include <libgnorba/gnorba.h>
#include "cdaudio.h"

GNOME_CDAudio cdaudio_client;

int main(int argc, char *argv[])
{
    CORBA_Environment ev;
    CORBA_ORB orb;
    CORBA_long rv;
    char buf[30];
    GoadServer *servlist, *chosen = NULL;
    int i;

    CORBA_exception_init(&ev);
    orb = gnome_CORBA_init("CDAudio Client", "0.0", &argc, argv, &ev);

    
    servlist = goad_server_list_get();
    for(i = 0; servlist[i].repo_id; i++) 
    {
	g_print("ID %d: %s\n", i, servlist[i].server_id);
	if(!strcmp(servlist[i].server_id, "cdaudio")) 
	{
	    g_print("Chosen: %s\n", servlist[i].server_id);
	    chosen = &servlist[i];
	    break;
	}
    }
    cdaudio_client = goad_server_activate(chosen, GOAD_ACTIVATE_SHLIB);

    if(cdaudio_client == CORBA_OBJECT_NIL)
    {
	fprintf(stderr,"Cannot activate server\n");
	exit(1);
    }

    g_print("%p\n", cdaudio_client);
    GNOME_CDAudio_init_cd(cdaudio_client, "/dev/cdrom", &ev);
    GNOME_CDAudio_play_cd(cdaudio_client, 1, 2, &ev);
 
    CORBA_Object_release(cdaudio_client, &ev);
    CORBA_Object_release((CORBA_Object)orb, &ev);

    return 0;
}

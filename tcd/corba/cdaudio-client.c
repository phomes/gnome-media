#include <stdio.h>
#include <orb/orbit.h>
#include <libgnorba/gnorba.h>
#include "cdaudio.h"

GNOME_CDAudio cdaudio_client;

void Exception (CORBA_Environment * ev);

void Exception (CORBA_Environment * ev)
{
    switch (ev->_major)
    {
    case CORBA_SYSTEM_EXCEPTION:
	g_log ("GNOME CDAudio Client", G_LOG_LEVEL_DEBUG, "CORBA system exception %s.\n",
	       CORBA_exception_id (ev));
	break;
    case CORBA_USER_EXCEPTION:
	g_log ("GNOME CDAudio Client", G_LOG_LEVEL_DEBUG, "CORBA user exception: %s.\n",
	       CORBA_exception_id (ev));                                             
	break;
    default:
	break;
    }
}


int main(int argc, char *argv[])
{
    CORBA_Environment ev;
    CORBA_ORB orb;
    GNOME_CDAudio_ErrorType rv;
    GNOME_CDAudio_CD *cd;
    
    char buf[30];
    GoadServerList *servlist, *chosen = NULL;
    int i;

    CORBA_exception_init(&ev);
    orb = gnome_CORBA_init("CDAudio Client", "0.0", &argc, argv, 0, &ev);
    /*  Exception(&ev); */

    cdaudio_client = goad_server_activate_with_repo_id(0, "IDL:GNOME/CDAudio:1.0", GOAD_ACTIVATE_REMOTE, NULL);

    if(cdaudio_client == CORBA_OBJECT_NIL)
    {
	fprintf(stderr,"Cannot activate server\n");
	exit(1);
    }

    cd = GNOME_CDAudio_init_cd(cdaudio_client, "/dev/cdrom", &ev);
    g_print("%p\n", cd);
    Exception(&ev);
    g_print("%p\n", cd);
    rv = GNOME_CDAudio_play_cd(cdaudio_client, cd, 1, 2, &ev);
    g_print("%ld\n", rv);
    Exception(&ev);
    g_print("%ld\n", rv);

    CORBA_Object_release(cdaudio_client, &ev);
/*    Exception(&ev); */
    CORBA_Object_release((CORBA_Object)orb, &ev);
/*    Exception(&ev); */
 
    return 0;
}

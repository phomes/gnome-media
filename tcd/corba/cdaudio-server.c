#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <orb/orbit.h>
#include <libgnorba/gnorba.h>
#include "cdaudio.h"

void Exception (CORBA_Environment * ev);

void Exception (CORBA_Environment * ev)
{
    switch (ev->_major)
    {
    case CORBA_SYSTEM_EXCEPTION:
	g_log ("Gnome CD Audio Server", G_LOG_LEVEL_DEBUG, "CORBA system exception %s.\n",
	       CORBA_exception_id (ev));                                              
	exit (1);
    case CORBA_USER_EXCEPTION:
	g_log ("Gnome CD Audio Server", G_LOG_LEVEL_DEBUG, "CORBA user exception: %s.\n",
	       CORBA_exception_id (ev));                                             
	exit (1);
    default:
	break;
    }
}

int main(int argc, char *argv[])
{
    CORBA_ORB orb;
    CORBA_Environment ev;
    CORBA_Object server;
    CORBA_Object name_server;
    PortableServer_POA poa;
    PortableServer_POAManager pm;
        
    CORBA_exception_init(&ev);
    orb = gnome_CORBA_init("cdaudio", "0.0", &argc, argv, GNORBA_INIT_SERVER_FUNC, &ev);
    Exception (&ev);

    poa = (PortableServer_POA)CORBA_ORB_resolve_initial_references(orb, "RootPOA", &ev);
    Exception (&ev);

    server = impl_GNOME_CDAudio__create(poa, &ev);
    g_print("server: %s\n", CORBA_ORB_object_to_string(orb, server, &ev));
    Exception (&ev);

    pm = PortableServer_POA__get_the_POAManager (poa, &ev);
    Exception (&ev);

    PortableServer_POAManager_activate (pm, &ev);
    Exception (&ev);

    name_server = gnome_name_service_get ();
    goad_server_register (name_server,
			  server,
			  "cdaudio",
			  "object",
			  &ev);
	
    CORBA_ORB_run(orb, &ev);
    return 0;
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <orb/orbit.h>
#include <libgnorba/gnorba.h>
#include "cdaudio.h"

GNOME_CDAudio cdaudio_client = CORBA_OBJECT_NIL;

static GNOME_CDAudio_Info *do_cdaudio_init_cd(PortableServer_Servant servant,
				       CORBA_char *path, 
				       CORBA_long *result,
				       CORBA_Environment *ev);
static CORBA_Object do_cdaudio_play_cd(PortableServer_Servant servant,
				       CORBA_long start_track,
				       CORBA_long end_track,
				       CORBA_long *result,
				       CORBA_Environment *ev);
static CORBA_Object do_cdaudio_pause_cd(PortableServer_Servant servant,
					CORBA_Environment *ev);
static CORBA_Object do_cdaudio_stop_cd(PortableServer_Servant servant,
				       CORBA_Environment *ev);
static CORBA_Object do_cdaudio_eject_cd(PortableServer_Servant servant,
					CORBA_Environment *ev);
static CORBA_Object do_cdaudio_get_time(PortableServer_Servant servant,
					GNOME_CDAudio_TimeFormat format,
					CORBA_short *min,
					CORBA_short *sec,
					CORBA_Environment *ev);
static CORBA_Object do_cdaudio_get_disc_info(PortableServer_Servant servant,
					     CORBA_char **artist,
					     CORBA_char **album,
					     CORBA_char **title,
					     CORBA_Environment *ev);

PortableServer_ServantBase__epv base_epv = {
  NULL,
  NULL,
  NULL
};

POA_GNOME_CDAudio__epv GNOME_CDAudio_epv = 
{ 
    NULL,
    (gpointer) &do_cdaudio_init_cd, 
    (gpointer) &do_cdaudio_play_cd,
    (gpointer) &do_cdaudio_pause_cd,
    (gpointer) &do_cdaudio_stop_cd,
    (gpointer) &do_cdaudio_eject_cd,
    (gpointer) &do_cdaudio_get_time,
    (gpointer) &do_cdaudio_get_disc_info
};

POA_GNOME_CDAudio__vepv poa_GNOME_CDAudio_vepv = { &base_epv, &GNOME_CDAudio_epv };
POA_GNOME_CDAudio poa_GNOME_CDAudio_servant = { NULL, &poa_GNOME_CDAudio_vepv };

typedef struct
{
    POA_GNOME_CDAudio servant;
    PortableServer_POA poa;
} impl_GNOME_CDAudio;

GNOME_CDAudio create_cdaudio(PortableServer_POA poa, CORBA_Environment * ev)
{
    GNOME_CDAudio retval;
    impl_GNOME_CDAudio *newservant;
    PortableServer_ObjectId *objid;
    
    newservant = g_new0(impl_GNOME_CDAudio, 1);
    newservant->servant.vepv = &poa_GNOME_CDAudio_vepv;
    newservant->poa = poa;
    POA_GNOME_CDAudio__init((PortableServer_Servant) newservant, ev);
    objid = PortableServer_POA_activate_object(poa, newservant, ev);
    CORBA_free(objid);
    retval = PortableServer_POA_servant_to_reference(poa, newservant, ev);
    
    return retval;
}    

void Exception (CORBA_Environment * ev);

void
Exception (CORBA_Environment * ev)
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

        server = create_cdaudio(poa, &ev);
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

static GNOME_CDAudio_Info *do_cdaudio_init_cd(PortableServer_Servant servant,
				   CORBA_char *path,
				   CORBA_long *result,
				   CORBA_Environment *ev)
{
    GNOME_CDAudio_Info *info;
    g_print("do_cdaudio_init_cd stub\n");

    info = GNOME_CDAudio_Info__alloc();
    info->path = "Foozle";
    info->fd = 42;
    info->status = 0;

    return info;
}


static CORBA_Object do_cdaudio_play_cd(PortableServer_Servant servant,
				   CORBA_long start_track,
				   CORBA_long end_track,
				   CORBA_long *result,
				   CORBA_Environment *ev)
{
    g_print("do_cdaudio_play_cd stub\n");
    *result = 69;
    return CORBA_Object_duplicate(cdaudio_client, ev);
}

static CORBA_Object do_cdaudio_pause_cd(PortableServer_Servant servant,
					CORBA_Environment *ev)
{
    g_print("do_cdaudio_pause_cd stub\n");   
    return CORBA_Object_duplicate(cdaudio_client, ev);
}

static CORBA_Object do_cdaudio_stop_cd(PortableServer_Servant servant,
				       CORBA_Environment *ev)
{
    g_print("do_cdaudio_stop_cd stub\n");
    return CORBA_Object_duplicate(cdaudio_client, ev);
}

static CORBA_Object do_cdaudio_eject_cd(PortableServer_Servant servant,
					CORBA_Environment *ev)
{
    g_print("do_cdaudio_eject_cd stub\n");
    return CORBA_Object_duplicate(cdaudio_client, ev);
}

static CORBA_Object do_cdaudio_get_time(PortableServer_Servant servant,
					GNOME_CDAudio_TimeFormat format,
					CORBA_short *min,
					CORBA_short *sec,
					CORBA_Environment *ev)
{
    g_print("do_cdaudio_get_time stub\n");
    return CORBA_Object_duplicate(cdaudio_client, ev);
}

static CORBA_Object do_cdaudio_get_disc_info(PortableServer_Servant servant,
					     CORBA_char **artist,
					     CORBA_char **album,
					     CORBA_char **title,
					     CORBA_Environment *ev)
{
    g_print("do_cdaudio_get_disc_info stub\n");
    return CORBA_Object_duplicate(cdaudio_client, ev);
}

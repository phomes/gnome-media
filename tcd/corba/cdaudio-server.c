#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <orb/orbit.h>
#include <libgnorba/gnorba.h>
#include "cdaudio.h"

GNOME_CDAudio cdaudio_client = CORBA_OBJECT_NIL;

static CORBA_Object do_cdaudio_init_cd(PortableServer_Servant servant,
				       CORBA_char *path, 
				       CORBA_Environment *ev);
static CORBA_Object do_cdaudio_play_cd(PortableServer_Servant servant,
				       CORBA_long track, 
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
    do_cdaudio_init_cd, 
    do_cdaudio_play_cd,
    do_cdaudio_pause_cd,
    do_cdaudio_stop_cd,
    do_cdaudio_eject_cd,
    do_cdaudio_get_time,
    do_cdaudio_get_disc_info
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

int main(int argc, char *argv[])
{
	FILE *fp;
    PortableServer_ObjectId objid = {0, sizeof("cdaudio_server"), "cdaudio_server"};
    PortableServer_POA poa;
    GNOME_CDAudio cdaudio;

    CORBA_Object name_service;
    CORBA_Environment ev;
    char *retval;
    CORBA_ORB orb;

    CORBA_exception_init(&ev);
    orb = gnome_CORBA_init("GNOME_CDAudio Server", "1.0", &argc, argv, GNORBA_INIT_SERVER_FUNC, &ev);

    POA_GNOME_CDAudio__init(&poa_GNOME_CDAudio_servant, &ev);
    
    poa = (PortableServer_POA)CORBA_ORB_resolve_initial_references(orb, "RootPOA", &ev);
    PortableServer_POAManager_activate(PortableServer_POA__get_the_POAManager(poa, &ev), &ev);
    
    PortableServer_POA_activate_object_with_id(poa,
					       &objid, &poa_GNOME_CDAudio_servant, &ev);
    
    cdaudio_client = PortableServer_POA_servant_to_reference(poa,
							     &poa_GNOME_CDAudio_servant,
							     &ev);

    cdaudio = create_cdaudio(poa, &ev);
    g_print("getting name_service..\n");
    name_service = gnome_name_service_get();
    g_print("registering...\n");
    goad_server_register(name_service, cdaudio, "CDAudio", "object", &ev);
    g_print("registered\n");


    retval = CORBA_ORB_object_to_string(orb, cdaudio, &ev);
    g_print("MYIOR: %s\n", retval);
    fprintf(stdout, "\n%s\n", retval);
    fflush(stdout);

    CORBA_free(retval);
    g_print("running ORB\n");
    CORBA_ORB_run(orb, &ev);
    fp = fopen("/home/timg/cvs/gnome-media/tcd/corba/blah.log", "w");
    fprintf(fp, "exiting...\n");   
    fclose(fp);
    return 0;
}

static CORBA_Object do_cdaudio_init_cd(PortableServer_Servant servant,
				   CORBA_char *path, 
				   CORBA_Environment *ev)
{
    g_print("do_cdaudio_init_cd stub\n");
    return CORBA_Object_duplicate(cdaudio_client, ev);
}


static CORBA_Object do_cdaudio_play_cd(PortableServer_Servant servant,
				   CORBA_long track, 
				   CORBA_Environment *ev)
{
    g_print("do_cdaudio_play_cd stub\n");
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

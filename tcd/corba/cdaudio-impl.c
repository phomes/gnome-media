#include "cdaudio.h"

/*** App-specific servant structures ***/
typedef struct {
   POA_GNOME_CDAudio servant;
   PortableServer_POA poa;

} impl_POA_GNOME_CDAudio;

/*** Implementation stub prototypes ***/
static void impl_GNOME_CDAudio__destroy(impl_POA_GNOME_CDAudio * servant,
					CORBA_Environment * ev);

GNOME_CDAudio_CD *
 impl_GNOME_CDAudio_init_cd(impl_POA_GNOME_CDAudio * servant,
			    CORBA_char * path,
			    CORBA_Environment * ev);

GNOME_CDAudio_ErrorType
impl_GNOME_CDAudio_play_cd(impl_POA_GNOME_CDAudio * servant,
			   GNOME_CDAudio_CD * cd,
			   CORBA_short start_track,
			   CORBA_short end_track,
			   CORBA_Environment * ev);

GNOME_CDAudio_ErrorType
impl_GNOME_CDAudio_pause_cd(impl_POA_GNOME_CDAudio * servant,
			    GNOME_CDAudio_CD * cd,
			    CORBA_Environment * ev);

GNOME_CDAudio_ErrorType
impl_GNOME_CDAudio_stop_cd(impl_POA_GNOME_CDAudio * servant,
			   GNOME_CDAudio_CD * cd,
			   CORBA_Environment * ev);

GNOME_CDAudio_ErrorType
impl_GNOME_CDAudio_eject_cd(impl_POA_GNOME_CDAudio * servant,
			    GNOME_CDAudio_CD * cd,
			    CORBA_Environment * ev);

GNOME_CDAudio_ErrorType
impl_GNOME_CDAudio_get_info(impl_POA_GNOME_CDAudio * servant,
			    GNOME_CDAudio_CD * cd,
			    GNOME_CDAudio_Info * info,
			    CORBA_Environment * ev);

GNOME_CDAudio_ErrorType
impl_GNOME_CDAudio_get_disc_titles(impl_POA_GNOME_CDAudio * servant,
				   GNOME_CDAudio_CD * cd,
				   CORBA_char ** artist,
				   CORBA_char ** album,
				   CORBA_char ** title,
				   CORBA_Environment * ev);

GNOME_CDAudio_ErrorType
impl_GNOME_CDAudio_get_track_title(impl_POA_GNOME_CDAudio * servant,
				   GNOME_CDAudio_CD * cd,
				   CORBA_short track,
				   CORBA_short * title,
				   CORBA_Environment * ev);

/*** epv structures ***/
static PortableServer_ServantBase__epv impl_GNOME_CDAudio_base_epv =
{
   NULL,			/* _private data */
   (gpointer) & impl_GNOME_CDAudio__destroy,	/* finalize routine */
   NULL,			/* default_POA routine */
};
static POA_GNOME_CDAudio__epv impl_GNOME_CDAudio_epv =
{
   NULL,			/* _private */

   (gpointer) & impl_GNOME_CDAudio_init_cd,

   (gpointer) & impl_GNOME_CDAudio_play_cd,

   (gpointer) & impl_GNOME_CDAudio_pause_cd,

   (gpointer) & impl_GNOME_CDAudio_stop_cd,

   (gpointer) & impl_GNOME_CDAudio_eject_cd,

   (gpointer) & impl_GNOME_CDAudio_get_info,

   (gpointer) & impl_GNOME_CDAudio_get_disc_titles,

   (gpointer) & impl_GNOME_CDAudio_get_track_title,

};

/*** vepv structures ***/
static POA_GNOME_CDAudio__vepv impl_GNOME_CDAudio_vepv =
{
   &impl_GNOME_CDAudio_base_epv,
   &impl_GNOME_CDAudio_epv,
};

/*** Stub implementations ***/
GNOME_CDAudio 
impl_GNOME_CDAudio__create(PortableServer_POA poa, CORBA_Environment * ev)
{
   GNOME_CDAudio retval;
   impl_POA_GNOME_CDAudio *newservant;
   PortableServer_ObjectId *objid;

   newservant = g_new0(impl_POA_GNOME_CDAudio, 1);
   newservant->servant.vepv = &impl_GNOME_CDAudio_vepv;
   newservant->poa = poa;
   POA_GNOME_CDAudio__init((PortableServer_Servant) newservant, ev);
   objid = PortableServer_POA_activate_object(poa, newservant, ev);
   CORBA_free(objid);
   retval = PortableServer_POA_servant_to_reference(poa, newservant, ev);

   return retval;
}

/* You shouldn't call this routine directly without first deactivating the servant... */
static void
impl_GNOME_CDAudio__destroy(impl_POA_GNOME_CDAudio * servant, CORBA_Environment * ev)
{

   POA_GNOME_CDAudio__fini((PortableServer_Servant) servant, ev);
   g_free(servant);
}

GNOME_CDAudio_CD *
impl_GNOME_CDAudio_init_cd(impl_POA_GNOME_CDAudio * servant,
			   CORBA_char * path,
			   CORBA_Environment * ev)
{
   GNOME_CDAudio_CD *retval = NULL;

   return retval;
}

GNOME_CDAudio_ErrorType
impl_GNOME_CDAudio_play_cd(impl_POA_GNOME_CDAudio * servant,
			   GNOME_CDAudio_CD * cd,
			   CORBA_short start_track,
			   CORBA_short end_track,
			   CORBA_Environment * ev)
{
   GNOME_CDAudio_ErrorType retval = 42;

   return retval;
}

GNOME_CDAudio_ErrorType
impl_GNOME_CDAudio_pause_cd(impl_POA_GNOME_CDAudio * servant,
			    GNOME_CDAudio_CD * cd,
			    CORBA_Environment * ev)
{
   GNOME_CDAudio_ErrorType retval;

   return retval;
}

GNOME_CDAudio_ErrorType
impl_GNOME_CDAudio_stop_cd(impl_POA_GNOME_CDAudio * servant,
			   GNOME_CDAudio_CD * cd,
			   CORBA_Environment * ev)
{
   GNOME_CDAudio_ErrorType retval;

   return retval;
}

GNOME_CDAudio_ErrorType
impl_GNOME_CDAudio_eject_cd(impl_POA_GNOME_CDAudio * servant,
			    GNOME_CDAudio_CD * cd,
			    CORBA_Environment * ev)
{
   GNOME_CDAudio_ErrorType retval;

   return retval;
}

GNOME_CDAudio_ErrorType
impl_GNOME_CDAudio_get_info(impl_POA_GNOME_CDAudio * servant,
			    GNOME_CDAudio_CD * cd,
			    GNOME_CDAudio_Info * info,
			    CORBA_Environment * ev)
{
   GNOME_CDAudio_ErrorType retval;

   return retval;
}

GNOME_CDAudio_ErrorType
impl_GNOME_CDAudio_get_disc_titles(impl_POA_GNOME_CDAudio * servant,
				   GNOME_CDAudio_CD * cd,
				   CORBA_char ** artist,
				   CORBA_char ** album,
				   CORBA_char ** title,
				   CORBA_Environment * ev)
{
   GNOME_CDAudio_ErrorType retval;

   return retval;
}

GNOME_CDAudio_ErrorType
impl_GNOME_CDAudio_get_track_title(impl_POA_GNOME_CDAudio * servant,
				   GNOME_CDAudio_CD * cd,
				   CORBA_short track,
				   CORBA_short * title,
				   CORBA_Environment * ev)
{
   GNOME_CDAudio_ErrorType retval;

   return retval;
}

/* This file is part of TCD 2.0.
   cdrom.c - CD DEVICE INTERFACE MODULE

   All the functions that start with tcd_ are here, and aren't
   (shouldn't be...) dependent on any user interface.

   Copyright (C) 1997-98 Tim P. Gerla <timg@means.net>
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
                   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
                                   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
                                               
   Tim P. Gerla
   RR 1, Box 40
   Climax, MN  56523
   timg@means.net
*/

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <sys/ioctl.h>

/* For _() */
#include <config.h>
#include <gnome.h>

#include "linux-cdrom.h"

#include "cddb.h"

#define FALSE (0)
#define TRUE (!FALSE)

/* make gcc happy */
void debug( const char *fmt, ...);

void debug( const char *fmt, ...)
{
#ifdef DEBUG
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
#endif
    return;
}

int tcd_init_disc( cd_struct *cd, WarnFunc msg_cb )
{
    char *homedir=NULL;
    
    debug("cdrom.c: tcd_init_disc(%p) top\n", cd );
    tcd_opencddev( cd, msg_cb );
    
    homedir = g_get_home_dir();
    if(!homedir)
	homedir = "/";
    
#if defined(TCD_CHANGER_ENABLED)
    cd->nslots = ioctl( cd->cd_dev, CDROM_CHANGER_NSLOTS );
#else
    cd->nslots = 0;
#endif
    cd->old_cddb_id = 0;
    
    debug("cdrom.c: tcd_init_disc exiting normally\n" );
    return(tcd_post_init(cd));
}

int tcd_post_init( cd_struct *cd )
{
    debug("cdrom.c: tcd_post_init(%p) top\n", cd );

    tcd_readtoc(cd);
    if(cd->cddb_id != cd->old_cddb_id)
    {		
	tcd_readdiskinfo(cd);
	cd->old_cddb_id = cd->cddb_id;
    }
    tcd_gettime(cd);

    if(cd->err)
    {
        debug( "cdrom.c: tcd_post_init exiting early (!cd->err)\n" );
        return(-1);
    }

    debug("cdrom.c: tcd_post_init exiting normally\n" );
    return(0);
}

int tcd_close_disc( cd_struct *cd )
{
    debug("cdrom.c: tcd_close_disc(%p) top\n", cd );
    close(cd->cd_dev);
    debug("cdrom.c: tcd_close_disc exiting normally\n" );
    return 0;
}

int tcd_readtoc( cd_struct *cd )
{
    int tmp,i;
    int delsecs;
    static int allocated_members;













































    debug("cdrom.c: tcd_readtoc(%p) top\n", cd );
    cd->err = FALSE;
    cd->isplayable=FALSE;
	
    /* read the TOC header */
    if(ioctl( cd->cd_dev, CDROMREADTOCHDR, &cd->tochdr))
    {
	strcpy( cd->errmsg, "Can't read disc." );
	cd->err = TRUE;
	debug("cdrom.c: tcd_readtoc exiting prematurly. CDROMREADTOCHDR ioctl error.\n" );
	cd->cur_t = 0;
	cd->cddb_id = 0;
	return(-1);		
    }

    /* grab first & last tracks */
    cd->first_t = cd->tochdr.cdth_trk0;
    cd->last_t = cd->tochdr.cdth_trk1;

    if(allocated_members < ((cd->last_t - cd->first_t)+2))
    {
	if(cd->trk)
	{
	    g_free(cd->trk);
	}
	cd->trk = g_new(struct cd_track, (cd->last_t - cd->first_t)+2);
	allocated_members = (cd->last_t - cd->first_t)+2;
    }

    /* read the leadout track */
    cd->trk[C(cd->last_t+1)].toc.cdte_track = CDROM_LEADOUT;
    cd->trk[C(cd->last_t+1)].toc.cdte_format = CDROM_MSF;

    /* read the leadout toc */
    if(ioctl(cd->cd_dev, CDROMREADTOCENTRY, &cd->trk[C(cd->last_t+1)].toc))
    {
	strcpy(cd->errmsg, "Can't read disc.");
	cd->err = TRUE;

	debug("cdrom.c: tcd_readtoc exiting prematurly. CDROMREADTOCENTRY ioctl error.\n" );
	cd->cur_t = 0;
	cd->cddb_id = 0;
	return(-1);
    }                                         

    /* read the rest of the tocs */
    for( i = cd->first_t; i <= cd->last_t; i++ )
    {
	cd->trk[C(i)].toc.cdte_track = i;
	cd->trk[C(i)].toc.cdte_format = CDROM_MSF;
	
	if(ioctl(cd->cd_dev, CDROMREADTOCENTRY, &cd->trk[C(i)].toc))
	{
	    strcpy( cd->errmsg, "Can't read disc." );
	    cd->err = TRUE;
	    debug("cdrom.c: tcd_readtoc exiting prematurly. CDROMREADTOCENTRY ioctl error.\n" );
	    cd->cur_t = 0;
	    cd->cddb_id = 0;
	    return(-1);
	}

	cd->trk[C(i)].type = cd->trk[C(i)].toc.cdte_ctrl;
	cd->trk[C(i)].length = cd->trk[C(i)].toc.cdte_addr.msf.minute * 60 +
	    cd->trk[C(i)].toc.cdte_addr.msf.second;
	cd->trk[C(i)].start = cd->trk[C(i)].length * 75 + 
	    cd->trk[C(i)].toc.cdte_addr.msf.frame;
    }

    /* calculate track times */
    for(i = cd->first_t; i <= cd->last_t; i ++)
    {
	/* Taken from cdtool...Thanks Thomas I.! */
	delsecs = cd->trk[C(i+1)].toc.cdte_addr.msf.minute * 60
	    + cd->trk[C(i+1)].toc.cdte_addr.msf.second
	    - cd->trk[C(i)].toc.cdte_addr.msf.minute * 60
	    - cd->trk[C(i)].toc.cdte_addr.msf.second;

	cd->trk[C(i)].tot_min = delsecs / 60;
	cd->trk[C(i)].tot_sec = delsecs - (delsecs/60)*60;

	if(cd->cddb_id != cd->old_cddb_id)
	{
		strcpy(cd->trk[C(i)].name, "(unknown)");
		cd->trk[C(i)].titled = FALSE;
	}
	else
		cd->trk[i].titled = TRUE;
    }	
    cd->trk[C(cd->last_t+1)].titled=TRUE;

    cd->cddb_id = cddb_discid(cd);

    cd->isplayable=TRUE;
    debug("cdrom.c: tcd_readtoc exiting normally\n" );
    return tmp;
}

void tcd_recalculate(cd_struct *cd)
{
    int result;

    /* calculate various timing values */
    cd->cur_pos_abs = cd->sc.cdsc_absaddr.msf.minute * 60 +
	cd->sc.cdsc_absaddr.msf.second;
    cd->cur_frame = cd->cur_pos_abs * 75 + cd->sc.cdsc_absaddr.msf.frame;
        
    cd->cur_pos_rel = (cd->cur_frame - cd->trk[C(cd->cur_t)].start) / 75;
	
    if (cd->cur_pos_rel < 0)
	cd->cur_pos_rel = -cd->cur_pos_rel;
        
    if (cd->cur_pos_rel > 0 && (result = cd->cur_pos_rel % 60) == cd->t_sec)
	return;

    /* calculate current track time */
    cd->t_sec = result;
    cd->t_min = cd->cur_pos_rel / 60;

    /* calculate current cd time */
    cd->cd_sec = cd->cur_pos_abs % 60;
    cd->cd_min = cd->cur_pos_abs / 60;

#ifdef TCD_CHANGER_ENABLED
    cd->cur_disc = ioctl( cd->cd_dev, CDROM_SELECT_DISC, CDSL_CURRENT );
#endif
}

void tcd_gettime( cd_struct *cd )
{
    cd->err = FALSE;
    cd->sc.cdsc_format = CDROM_MSF;
        
    if(cd->isplayable)
    {
	if(ioctl( cd->cd_dev, CDROMSUBCHNL, &cd->sc))
	{
	    strcpy( cd->errmsg, "Can't read disc." );
	    cd->err = TRUE;
	    debug("cdrom.c: tcd_gettime exiting early. CDROMSUBCHNL ioctl error.\n" );
	    cd->cur_t = 0;
	    return;
	}
	if(cd->sc.cdsc_audiostatus==CDROM_AUDIO_PLAY)
		cd->cur_t = cd->sc.cdsc_trk;
	else
		cd->cur_t = 0;
	tcd_recalculate(cd);
    }
}

int tcd_set_volume(cd_struct *cd, int volume)
{
    struct cdrom_volctrl vol;

    vol.channel0 = volume;
    vol.channel1 = vol.channel2 = vol.channel3 = vol.channel0;
	
    if(ioctl(cd->cd_dev, CDROMVOLCTRL, &vol) < 0)
	return FALSE;

    return TRUE;
}

int tcd_get_volume(cd_struct *cd)
{
#ifdef CDROMVOLREAD
    struct cdrom_volctrl vol;

    if(ioctl(cd->cd_dev, CDROMVOLREAD, &vol) < 0)
	return -1;

    return vol.channel0;
#else
    return 0;
#endif
}	
	                                  
int tcd_playtracks(cd_struct *cd, int start_t, int end_t, int only_use_trkind)
{
    struct cdrom_msf msf;
    struct cdrom_ti trkind;
    int tmp;
    debug("cdrom.c: tcd_playtracks( %p, %d, %d )\n", cd, start_t, end_t );
    cd->err = FALSE;
	
    /* make sure we can play it */
    tcd_gettime(cd);
    if(cd->err) 
    {
	/* try and inject cd */
	tcd_ejectcd(cd);

	if(cd->err) 
	{
	    debug("cdrom.c: tcd_playtracks - error while fetching disc.\n");
	    return -1;
	}
    }

#if defined(CDROMCLOSETRAY)
    ioctl(cd->cd_dev, CDROMCLOSETRAY);
#endif	        
	
    if(cd->trk[start_t].toc.cdte_ctrl == CDROM_DATA_TRACK)
	start_t++;		/* bad hack. most data tracks are the first track... */

    msf.cdmsf_min0 = cd->trk[start_t].toc.cdte_addr.msf.minute;
    msf.cdmsf_sec0 = cd->trk[start_t].toc.cdte_addr.msf.second;
    msf.cdmsf_frame0 = cd->trk[start_t].toc.cdte_addr.msf.frame;
	
    if( end_t < 0 )
    {
	msf.cdmsf_min1 = cd->trk[start_t].tot_min+msf.cdmsf_min0;
	msf.cdmsf_sec1 = cd->trk[start_t].tot_sec+msf.cdmsf_sec0;
	msf.cdmsf_frame1=0;
    }
    else
    {
	msf.cdmsf_min1 = cd->trk[end_t+1].toc.cdte_addr.msf.minute;
	msf.cdmsf_sec1 = cd->trk[end_t+1].toc.cdte_addr.msf.second;
	msf.cdmsf_frame1 = cd->trk[end_t+1].toc.cdte_addr.msf.frame - 1;

#ifdef UNSIGNED_NUMBERS_CAN_BE_NEGATIVE
	if(msf.cdmsf_frame1 < 0)
	{
	    msf.cdmsf_sec1 += msf.cdmsf_frame1;
	    msf.cdmsf_frame1 = 0;
	}
	if(msf.cdmsf_sec1 < 0)
	{
	    msf.cdmsf_min1 += msf.cdmsf_sec1;
	    msf.cdmsf_sec1 = 0;
	}
	if(msf.cdmsf_min1 < 0)
	{
	    msf.cdmsf_min1 = 0;
	}
#endif
    }
    msf.cdmsf_min1 += (msf.cdmsf_sec1 / 60);
    msf.cdmsf_sec1 %= 60;

    if(ioctl( cd->cd_dev, CDROMPLAYMSF, &msf) || only_use_trkind)
    {
	debug("cdrom.c: tcd_playtracks error. CDROMPLAYMSF ioctl error (or user override). Trying PLAYTRKIND\n" );

	/* Try alternate method of playing */
	trkind.cdti_trk0 = start_t;     /* start track */
	trkind.cdti_ind0 = 0;      	/* start index */
	trkind.cdti_trk1 = end_t;      	/* end track */
	trkind.cdti_ind1 = 0;      	/* end index */
		                                
	if(ioctl(cd->cd_dev, CDROMPLAYTRKIND, &trkind))
	{
	    strcpy( cd->errmsg, "Error playing disc" );
	    cd->err = TRUE;
	    debug("cdrom.c: tcd_playtracks error. CDROMPLAYTRKIND ioctl error.\n");
	    return -1;
	}
    }

    cd->isplayable = TRUE;
    debug("cdrom.c: tcd_playtracks exiting normally\n" );
    return tmp;
}       

int tcd_play_seconds( cd_struct *cd, long int offset )
{
    struct cdrom_msf msf;
    int tmp;

    debug("cdrom.c: tcd_playseconds( %p, %ld )\n", cd, offset );

    cd->err = FALSE;
    cd->isplayable=FALSE;

    /* got subchannel? */
    msf.cdmsf_sec0 = cd->sc.cdsc_absaddr.msf.second+offset;
    msf.cdmsf_min0 = cd->sc.cdsc_absaddr.msf.minute;
    msf.cdmsf_frame0 = cd->sc.cdsc_absaddr.msf.frame;
    msf.cdmsf_min1 = cd->trk[C(cd->last_t+1)].toc.cdte_addr.msf.minute;
    msf.cdmsf_sec1 = cd->trk[C(cd->last_t+1)].toc.cdte_addr.msf.second;
    msf.cdmsf_frame1 = cd->trk[C(cd->last_t+1)].toc.cdte_addr.msf.frame - 1;

#ifdef UNSIGNED_NUMBERS_CAN_BE_NEGATIVE
    if(msf.cdmsf_frame1 < 0)
    {
	msf.cdmsf_sec1 += msf.cdmsf_frame1;
	msf.cdmsf_frame1 = 0;
    }
    if(msf.cdmsf_sec1 < 0)
    {
	msf.cdmsf_min1 += msf.cdmsf_sec1;
	msf.cdmsf_sec1 = 0;
    }
    if(msf.cdmsf_min1 < 0)
    {
	msf.cdmsf_min1 = 0;
    }
#endif
	
    if( msf.cdmsf_sec0 > 60 && (offset<0) )
    {
	msf.cdmsf_sec0 = 60-abs(offset);
	msf.cdmsf_min0--;
    }
	
    if(ioctl(cd->cd_dev, CDROMPLAYMSF, &msf))
    {
	strcpy( cd->errmsg, "Error playing disc." );
	cd->err = TRUE;

	debug("cdrom.c: tcd_play_seconds error. CDROMPLAYMSF ioctl error.\n" );
	return(-1);
    }
    cd->isplayable=TRUE;                                                 

    debug("cdrom.c: tcd_playseconds exiting normally\n" );
    return tmp;
}       

int tcd_ejectcd( cd_struct *cd )
{
    int tmp;

    debug("cdrom.c: tcd_eject(%p) top\n", cd );
    if(cd->isplayable) tcd_stopcd(cd);
    cd->err = FALSE;

    if(!ioctl(cd->cd_dev, CDROMEJECT))
    {
	cd->isplayable = FALSE;
	strcpy(cd->errmsg, "No disc in drive ");
	cd->err = TRUE;
    } 
    else 
    {
#ifdef CDROMCLOSETRAY
	tmp = ioctl( cd->cd_dev, CDROMCLOSETRAY );
#endif

	if(tcd_post_init(cd))
	{
	    strcpy( cd->errmsg, "Disc init error. " );
	    cd->err = TRUE;
			
	    debug("cdrom.c: tcd_eject - disc init error. %s\n",  
		  strerror(errno) );

	    return(-1);
	}
	cd->isplayable = TRUE;
    }
    cd->cur_t = 0;	
	
    debug("cdrom.c: tcd_eject exiting normally\n" );
    return 0;
}       

int tcd_stopcd(cd_struct *cd)
{
    int tmp;

    debug("cdrom.c: tcd_stopcd(%p)\n", cd );
	
    /* SDH: Makes things cleaner on eject */
    if( cd->sc.cdsc_audiostatus==CDROM_AUDIO_PAUSED )
	tcd_pausecd(cd);

    cd->err = FALSE;
    if(ioctl(cd->cd_dev, CDROMSTOP))
    {
	strcpy( cd->errmsg, "Can't stop disc." );
	cd->err = TRUE;

	debug("cdrom.c: tcd_stopcd exiting early. CDROMSTOP ioctl error = %x.\n", tmp);
	return(-1);
    }

    debug("cdrom.c: tcd_stopcd exiting normally\n" );
    return tmp;
}       

int tcd_pausecd( cd_struct *cd )
{
    int tmp;
    cd->err = FALSE;
	
    if(cd->sc.cdsc_audiostatus==CDROM_AUDIO_PAUSED)
    {       
	if((tmp=ioctl(cd->cd_dev, CDROMRESUME)))
	{
	    strcpy(cd->errmsg, strerror(errno));
	    cd->err = TRUE;
	    return(-1);
	}
	return tmp;
    }	        
    else
    {
	if((tmp=ioctl(cd->cd_dev, CDROMPAUSE)))
	{
	    strcpy( cd->errmsg, strerror( errno ) );
	    cd->err = TRUE;
	    return(-1);
	}
	return tmp;
    }
}

int tcd_change_disc( cd_struct *cd, int disc )
{
#ifdef TCD_CHANGER_ENABLED
    int tmp;
    cd->err = FALSE;

    tmp = ioctl( cd->cd_dev, CDROM_SELECT_DISC, disc );
    if(tmp && errno)
	fprintf( stdout, "ioctl: %s\n", strerror(errno) );	

    return tmp;
#else
    debug("tcd_change_disc called, but changer support isn't compiled in. Ickyblah.\n" );
    return 0;
#endif
}
	                   
void tcd_opencddev( cd_struct *cd, WarnFunc msg_cb )
{
    char tmp[256];
    cd->err = FALSE;

    if(cd->cd_dev > 0) /* SDH rvs test (was < should be > ) */
	close(cd->cd_dev);
                                                
    cd->cd_dev = open(cd->cdpath, O_RDONLY | O_NONBLOCK);
 
    if(cd->cd_dev < 0)
    {
	g_snprintf(tmp, 255, "Error accessing cdrom device.\n"
		             "Please check to make sure cdrom drive support\n"
                             "is compiled into the kernel, and that you have\n"
                             "permission to access the device.\n\nReason: %s\n", strerror(errno));
	if(msg_cb)
	    msg_cb(tmp,"error");
	cd->err = TRUE;
	return;
    }
}

void parse_dtitle(cd_struct *cd)
{
    char tmp[DISC_INFO_LEN], *tmp2;

    /* Parse out the individual title elements. Help from Alec M. */
    strncpy(tmp, cd->dtitle, DISC_INFO_LEN);
    if (tmp[0])
	strncpy(cd->artist, strtok(tmp, "/"), DISC_INFO_LEN);
    else
	cd->artist[0]=0;

    tmp2 = strtok(NULL, "\0");
    if(tmp2)
	strncpy(cd->album, tmp2+1, DISC_INFO_LEN);
    else
	strncpy(cd->album, "", DISC_INFO_LEN);
}

int tcd_readdiskinfo( cd_struct *cd )
{
    int i, res;
    FILE *fp;
    char fn[60];
    char *homedir=NULL;
    char tcd_dir[128];
    
    if( !cd->isplayable )
	return 0;
    
    homedir = g_get_home_dir();
    if(!homedir)
	homedir = "/";

    strncpy( tcd_dir, homedir, sizeof(tcd_dir) );
    strncat( tcd_dir, "/.cddbslave/", sizeof(tcd_dir) );
    tcd_dir[sizeof(tcd_dir) - 1] = 0;
    
    g_snprintf( fn, sizeof(fn) - 1, "%s%08lx", tcd_dir, cd->cddb_id );

    fp = fopen(fn, "r");	
    if(fp != NULL)
    {
	fclose(fp);
	if((res = tcd_readcddb( cd, fn ))<0)
	{
	    debug("tcd_readcddb returned an error, %d\n", res );
	    return -1;
	}

        parse_dtitle(cd);
	strncpy(cd->trk[0].name, "--", TRK_NAME_LEN);
	return 0;
    }
    else
    {
	/* Here's where we want to send a request to our slave */
	tcd_call_cddb_slave(cd, "TCD", "1.0");
	debug("Warning, can't open \'%s\' \n", fn );
	strcpy( cd->dtitle, _("Unknown / Unknown") );
	
	for( i = cd->first_t; i <= cd->last_t; i++ )
	{
	    sprintf( cd->trk[C(i)].name, _("Track %d"), i );
	    cd->trk[C(i)].titled = FALSE;
	}
	strcpy( cd->trk[0].name, "--" );

	parse_dtitle(cd);
	strncpy(cd->trk[0].name, "--", TRK_NAME_LEN);

	return 0;
    }
    fclose(fp);
    return -1;
}

void tcd_writediskinfo( cd_struct *cd )
{
    char fn[60];
    char home[60];

    char *homedir = g_get_home_dir();
    
    if(!homedir)
	homedir = "/";
    
    strncpy( home, homedir, sizeof(home) );
    home[sizeof(home) - 1] = 0;
    g_snprintf( fn, sizeof(fn) - 1, "%s/.cddbslave/%08lx", home, cd->cddb_id );
    
    if(tcd_writecddb(cd, fn) < 0)
    {
	perror( "tcd_writecddb" );
	exit(-1);
    }
    
    cd->needs_dbwrite=FALSE;        
    return;
}					

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

#define DATABASE_SUPPORT

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <sys/ioctl.h>

#ifdef linux
# include <linux/cdrom.h>
# include "linux-cdrom.h"
#else
# error TCD currently only builds under Linux systems.
#endif

#include "cddb.h"

#define FALSE (0)
#define TRUE (!FALSE)

int tcd_init_disc( cd_struct *cd, WarnFunc msg_cb )
{
	char tcd_dir[60];
	char tmp[256];
	struct stat fs;
	struct cdrom_volctrl vol;

#ifdef DEBUG
	fprintf( stderr, "cdrom.c: tcd_init_disc(%p) top\n", cd );
#endif
	tcd_opencddev( cd, msg_cb );

#ifdef DATABASE_SUPPORT
	strcpy( tcd_dir, getenv("HOME"));
	strcat( tcd_dir, "/.tcd/" );
	if( stat( tcd_dir, &fs ) < 0 ) 
	{
		if( errno == ENOENT )
		{
			if( mkdir(tcd_dir,S_IRWXU) )
			{
				snprintf( tmp, 255, "Error creating %s. Reason: %s\n", tcd_dir, strerror(errno) );
				if( msg_cb )
					msg_cb(tmp,"error");
#ifdef DEBUG
				fprintf( stderr, "cdrom.c: tcd_init_disc exiting early.\n" );
#endif
				return -1;
			}
			if( msg_cb )
				msg_cb("Created directory ~/.tcd/ for cddb files.", "info" );
		}
		else 
		{
			snprintf( tmp, 255, "Error accessing %s. Reason: %s\n", tcd_dir, strerror(errno) );
			if( msg_cb )
				msg_cb(tmp,"error");
			return -1;
		}
	}
#endif
	
	if( !cd->err ) tcd_readtoc(cd);
#ifdef DATABASE_SUPPORT
	if( !cd->err ) tcd_readdiskinfo(cd);
#endif
	if( cd->err )
	{
#ifdef DEBUG
		fprintf( stderr, "cdrom.c: tcd_init_disc exiting early (!cd->err)\n" );
#endif
		return(-1);
	}

#ifdef TCD_CHANGER_ENABLED
	/* FIXME set capability flag here for CHANGER */
	cd->nslots = ioctl( cd->cd_dev, CDROM_CHANGER_NSLOTS );
#else
	cd->nslots = 0;
#endif
	ioctl( cd->cd_dev, CDROMVOLREAD, &vol );
	cd->volume = vol.channel0;

#ifdef DEBUG
	fprintf( stderr, "cdrom.c: tcd_init_disc exiting normally\n" );
#endif
	return(0);
}

int tcd_close_disc( cd_struct *cd )
{
#ifdef DEBUG
	fprintf( stderr, "cdrom.c: tcd_close_disc(%p) top\n", cd );
#endif
	close( cd->cd_dev );
#ifdef DEBUG
	fprintf( stderr, "cdrom.c: tcd_close_disc exiting normally\n" );
#endif
	return 0;
}

int tcd_readtoc( cd_struct *cd )
{
	int tmp,i;
	int delsecs;

#ifdef DEBUG
	fprintf( stderr, "cdrom.c: tcd_readtoc(%p) top\n", cd );
#endif
	cd->err = FALSE;
	cd->isplayable=FALSE;
	
	tmp = ioctl( cd->cd_dev, CDROMREADTOCHDR, &cd->tochdr );
	if( tmp < 0 )
	{
		strcpy( cd->errmsg, "Can't read disc." );
		cd->err = TRUE;
#ifdef DEBUG
		fprintf( stderr, "cdrom.c: tcd_readtoc exiting prematurly. CDROMREADTOCHDR ioctl error.\n" );
#endif
		return(-1);		
	}
	
	cd->first_t = cd->tochdr.cdth_trk0;
	cd->last_t = cd->tochdr.cdth_trk1;


	/* Ah ha! I was forgetting to read the leadout track! 
	   My timings were all screwed up. Thanks to the cdtool
	   source, I figured it out.
	*/
	cd->old_id = 0; /* Prevent errors :P */

    	cd->trk[C(cd->last_t+1)].toc.cdte_track = CDROM_LEADOUT;
        cd->trk[C(cd->last_t+1)].toc.cdte_format = CDROM_MSF;
	tmp = ioctl(cd->cd_dev, CDROMREADTOCENTRY, &cd->trk[C(cd->last_t+1)].toc );
	if( tmp < 0 )
	{
		strcpy( cd->errmsg, "Can't read disc." );
                cd->err = TRUE;

#ifdef DEBUG
                fprintf( stderr, "cdrom.c: tcd_readtoc exiting prematurly. CDROMREADTOCENTRY ioctl error.\n" );
#endif
		return(-1);
        }                                         

	for( i = cd->first_t; i <= cd->last_t; i++ )
	{
		cd->trk[C(i)].toc.cdte_track = i;
                cd->trk[C(i)].toc.cdte_format = CDROM_MSF;

                tmp = ioctl( cd->cd_dev, CDROMREADTOCENTRY, &cd->trk[C(i)].toc );		
                if( tmp < 0 )
		{
			strcpy( cd->errmsg, "Can't read disc." );
                  	cd->err = TRUE;
#ifdef DEBUG
	                fprintf( stderr, "cdrom.c: tcd_readtoc exiting prematurly. CDROMREADTOCENTRY ioctl error.\n" );
#endif
			return(-1);
                }

		cd->trk[C(i)].length = cd->trk[C(i)].toc.cdte_addr.msf.minute * 60 +
                        	    cd->trk[C(i)].toc.cdte_addr.msf.second;
                cd->trk[C(i)].start = cd->trk[C(i)].length * 75 + 
                                   cd->trk[C(i)].toc.cdte_addr.msf.frame;
		
		/* Must not change */
		cd->old_id += cd->trk[C(i)].length * 
			       (cd->trk[C(cd->last_t+1)].toc.cdte_addr.msf.minute*60+
		                cd->trk[C(cd->last_t+1)].toc.cdte_addr.msf.second);

		/* Set up the default playlist */
		cd->playlist[i] = i;
	}
	cd->playlist[i] = -1;
	
	for( i = cd->first_t; i <= cd->last_t; i ++ )
	{
		/* Taken from cdtool...Thanks Thomas I.! */
		delsecs = cd->trk[C(i+1)].toc.cdte_addr.msf.minute * 60
		+ cd->trk[C(i+1)].toc.cdte_addr.msf.second
		- cd->trk[C(i)].toc.cdte_addr.msf.minute * 60
		- cd->trk[C(i)].toc.cdte_addr.msf.second;

		cd->trk[C(i)].tot_min = delsecs / 60;
		cd->trk[C(i)].tot_sec = delsecs - (delsecs/60)*60;

		strcpy( cd->trk[C(tmp)].name, "(unknown)" );
		cd->trk[C(tmp)].titled = FALSE;
	}	
	cd->trk[C(cd->last_t+1)].titled=TRUE;

#ifdef DATABASE_SUPPORT
	cd->cddb_id = cddb_discid(cd);
#endif

	cd->isplayable=TRUE;
#ifdef DEBUG
	fprintf( stderr, "cdrom.c: tcd_readtoc exiting normally\n" );
#endif
	return tmp;
}

void tcd_gettime( cd_struct *cd )
{
	int result;
	struct cdrom_volctrl vol;

	cd->err = FALSE;
        cd->sc.cdsc_format = CDROM_MSF;
        	
	result = ioctl( cd->cd_dev, CDROMSUBCHNL, &cd->sc );
	if( result < 0 )
	{
		strcpy( cd->errmsg, "Can't read disc." );
               	cd->err = TRUE;
#ifdef DEBUG
		fprintf( stderr, "cdrom.c: tcd_gettime exiting early. CDROMSUBCHNL ioctl error.\n" );
#endif
		return;
        }

	vol.channel0 = cd->volume;
	vol.channel1 = vol.channel2 = vol.channel3 = vol.channel0; 
	ioctl( cd->cd_dev, CDROMVOLCTRL, &vol );
	ioctl( cd->cd_dev, CDROMVOLREAD, &vol );
	cd->volume = vol.channel0;

	cd->cur_t = cd->sc.cdsc_trk;

	cd->cur_pos_abs = cd->sc.cdsc_absaddr.msf.minute * 60 +
         cd->sc.cdsc_absaddr.msf.second;
        cd->cur_frame = cd->cur_pos_abs * 75 + cd->sc.cdsc_absaddr.msf.frame;
        
	cd->cur_pos_rel = (cd->cur_frame - cd->trk[C(cd->cur_t)].start) / 75;
	
	if (cd->cur_pos_rel < 0)
        	cd->cur_pos_rel = -cd->cur_pos_rel;
        
  	if (cd->cur_pos_rel > 0 && (result = cd->cur_pos_rel % 60) == cd->t_sec)
      		return;

        cd->t_sec = result;
        cd->t_min = cd->cur_pos_rel / 60;
        
        cd->cd_sec = cd->cur_pos_abs % 60;
	cd->cd_min = cd->cur_pos_abs / 60;

	/* Update cd->trk.status */
	for( result = cd->first_t; result <= cd->last_t; result++ )
	{
		cd->trk[C(result)].status = 0;
		if( (result == cd->cur_t) && cd->sc.cdsc_audiostatus == CDROM_AUDIO_PLAY )
			cd->trk[C(result)].status = TRK_PLAYING;
		if( result == cd->repeat_track )
			cd->trk[C(result)].status = TRK_REPEAT;
		if( cd->trk[C(result)].toc.cdte_ctrl == CDROM_DATA_TRACK )
			cd->trk[C(result)].status = TRK_DATA;
	}			
#ifdef TCD_CHANGER_ENABLED
	cd->cur_disc = ioctl( cd->cd_dev, CDROM_SELECT_DISC, CDSL_CURRENT );
#endif
}
	                                   
int tcd_playtracks( cd_struct *cd, int start_t, int end_t )
{
	int tmp;
#ifdef DEBUG
	fprintf( stderr, "cdrom.c: tcd_playtracks( %p, %d, %d )\n", cd, start_t, end_t );
#endif
	cd->err = FALSE;
	cd->isplayable=FALSE;
	cd->isdisk=FALSE;
	
	if( cd->trk[C(start_t)].status == TRK_DATA )
	{
#ifdef DEBUG
		fprintf( stderr, "cdrom.c: tcd_playtracks error. trying to play data track.\n" );
#endif
		return -1;
	}
	
        cd->ti.cdti_trk0 = start_t;   /* start track */
   	cd->ti.cdti_ind0 = 1;   /* start index */
        cd->ti.cdti_trk1 = end_t;  /* end track */
        cd->ti.cdti_ind1 = 1;   /* end index */
                                
        tmp = ioctl( cd->cd_dev, CDROMPLAYTRKIND, &cd->ti );
   	if( tmp < 0 )
	{
		strcpy( cd->errmsg, "Error playing disc" );
               	cd->err = TRUE;
#ifdef DEBUG
		fprintf( stderr, "cdrom.c: tcd_playtracks error. CDROMPLAYTRKIND ioctl error.\n" );
#endif
		return(-1);
        }
   	cd->isplayable=TRUE;                                                 
	cd->isdisk=TRUE;
#ifdef DEBUG
	fprintf( stderr, "cdrom.c: tcd_playtracks exiting normally\n" );
#endif
   	return tmp;
}       

int tcd_play_seconds( cd_struct *cd, long int offset )
{
	struct cdrom_msf msf;
	int tmp;
#ifdef DEBUG
	fprintf( stderr, "cdrom.c: tcd_playseconds( %p, %ld )\n", cd, offset );
#endif
	cd->err = FALSE;
	cd->isplayable=FALSE;
	cd->isdisk=FALSE;

	/* got subchannel? */
	msf.cdmsf_sec0 = cd->sc.cdsc_absaddr.msf.second+offset;
	msf.cdmsf_min0 = cd->sc.cdsc_absaddr.msf.minute;
	msf.cdmsf_frame0 = cd->sc.cdsc_absaddr.msf.frame;
	msf.cdmsf_min1 = cd->trk[C(cd->last_t+1)].toc.cdte_addr.msf.minute;
	msf.cdmsf_sec1 = cd->trk[C(cd->last_t+1)].toc.cdte_addr.msf.second;
	msf.cdmsf_frame1 = cd->trk[C(cd->last_t+1)].toc.cdte_addr.msf.frame;
	if( msf.cdmsf_sec0 > 60 && (offset<0) )
	{
		msf.cdmsf_sec0 = 60-abs(offset);
		msf.cdmsf_min0--;
	}
	
        tmp = ioctl( cd->cd_dev, CDROMPLAYMSF, &msf );
   	if( tmp < 0 )
	{
		strcpy( cd->errmsg, "Error playing disc." );
               	cd->err = TRUE;
#ifdef DEBUG
		fprintf( stderr, "cdrom.c: tcd_play_seconds error. CDROMPLAYMSF ioctl error.\n" );
#endif
		return(-1);
        }
   	cd->isplayable=TRUE;                                                 
	cd->isdisk=TRUE;
#ifdef DEBUG
	fprintf( stderr, "cdrom.c: tcd_playseconds exiting normally\n" );
#endif
   	return tmp;
}       

int tcd_ejectcd( cd_struct *cd )
{
	int tmp;
#ifdef DEBUG
	fprintf( stderr, "cdrom.c: tcd_eject(%p) top\n", cd );
#endif
	cd->err = FALSE;

	tmp = ioctl( cd->cd_dev, CDROMEJECT );
	/* Error? try injecting */
	if( tmp < 0 )
		tmp = ioctl( cd->cd_dev, CDROMCLOSETRAY );

   	if( tmp < 0 )
	{
		strcpy( cd->errmsg, "Can't eject disc." );
               	cd->err = TRUE;
#ifdef DEBUG
		perror( "ioctl" );
		fprintf( stderr, "cdrom.c: tcd_eject exiting early. eject error.\n", cd );
#endif
		return(-1);
        }
	cd->isplayable=FALSE;
	cd->isdisk=FALSE;

#ifdef DEBUG
	fprintf( stderr, "cdrom.c: tcd_eject exiting normally\n" );
#endif
   	return 0;
}       

int tcd_stopcd( cd_struct *cd )
{
	int tmp;
	cd->err = FALSE;
#ifdef DEBUG
	fprintf( stderr, "cdrom.c: tcd_stopcd(%p)\n", cd );
#endif

        tmp = ioctl( cd->cd_dev, CDROMSTOP );
   	if( tmp < 0 )
	{
		strcpy( cd->errmsg, "Can't stop disc." );
               	cd->err = TRUE;
#ifdef DEBUG
	fprintf( stderr, "cdrom.c: tcd_stopcd exiting early. CDROMSTOP ioctl error.\n" );
#endif
		return(-1);
        }
#ifdef DEBUG
        fprintf( stderr, "cdrom.c: tcd_stopcd exiting normally\n" );
#endif
           	return tmp;
}       

int tcd_pausecd( cd_struct *cd )
{
	int tmp;
	cd->err = FALSE;
	
	if( cd->sc.cdsc_audiostatus==CDROM_AUDIO_PAUSED )
	{       
		tmp = ioctl( cd->cd_dev, CDROMRESUME );
		if( tmp < 0 )
		{
			strcpy( cd->errmsg, strerror( errno ) );
                        cd->err = TRUE;
                        return(-1);
		}
	        return tmp;
	}	        
	else
	{
		tmp = ioctl( cd->cd_dev, CDROMPAUSE );
		if( tmp < 0 )
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
	int tmp, fd;
	cd->err = FALSE;
	fd = open( cd->cdpath, O_RDONLY | O_NONBLOCK );

        tmp = ioctl( fd, CDROM_SELECT_DISC, disc );
	if( tmp < 0 )
		fprintf( stdout, "ioctl: %s\n", strerror(errno) );	
	/* Don't be noisy if there's an error */

	close(fd);
   	return tmp;
#else
	fprintf( stderr, "tcd_change_disc called, but changer support isn't compiled in. Ickyblah.\n" );
	return 0;
#endif
}
	                   
void tcd_opencddev( cd_struct *cd, WarnFunc msg_cb )
{
	char tmp[256];
	cd->err = FALSE;
	cd->isdisk=FALSE;

	if( cd->cd_dev < 0 )
		close( cd->cd_dev );
                                                
	cd->cd_dev = open( cd->cdpath, O_RDONLY | O_NONBLOCK );
 
        if( cd->cd_dev < 0 )
        {
		snprintf( tmp, 255, "Error accessing cdrom device.\n\
			Please check to make sure cdrom drive support\n\
			is compiled into the kernel, and that you have\n\
			permission to access the device.\n\nReason: %s\n", strerror(errno));
		if( msg_cb )
			msg_cb(tmp,"error");
		cd->err = TRUE;
                return;
        }
	cd->isdisk=TRUE;
}

int tcd_readdiskinfo( cd_struct *cd )
{
	struct stat fs;
	int i, res;
	FILE *fp;
	char fn[60];
	char artist[DISC_INFO_LEN], album[DISC_INFO_LEN];
	char tcd_dir[128];
	
	strcpy( tcd_dir, getenv("HOME"));
        strcat( tcd_dir, "/.tcd/" );
                 
	sprintf( fn, "%s%08lx", tcd_dir, cd->cddb_id );
	fp = fopen(fn, "r");	
	if( fp != NULL ) // Does the new format exist?
	{
		fclose(fp);
		if( (res = tcd_readcddb( cd, fn ))<0 )
		{
			fprintf( stderr, "tcd_readcddb returned an error, %d\n", res );
			sleep(2);
			return -1;
		}
		strcpy( cd->trk[0].name, "--" );
		return 0;
	}
	else
	{

		sprintf( fn, "%s%ld.tcd", tcd_dir, cd->old_id );
#ifdef DEBUG
		fprintf( stderr,"Warning, can't open new format \'%s\', trying old.\n", fn );
		fprintf( stderr, "opening...%s\n", fn );
#endif
		if( !stat(fn, &fs) )
		{
			fp = fopen( fn, "r" );
			fgets( artist, 40, fp );
			fgets( album, 40, fp );
			artist[strlen(artist)-1] = 0;
			album[strlen(album)-1] = 0;
			
			strcpy( cd->dtitle, "" );
			strcat( cd->dtitle, artist );
			strcat( cd->dtitle, " / " );
			strcat( cd->dtitle, album );
				
			for( i = cd->first_t; i <= cd->last_t; i++ )
			{
				fgets( cd->trk[C(i)].name, 40, fp );
			        cd->trk[C(i)].name[strlen(cd->trk[C(i)].name)-1] = 0;
				cd->trk[C(i)].titled = TRUE;
			}
			sprintf( fn, "%s%08lx", tcd_dir, cd->cddb_id );
			tcd_writecddb( cd, fn );
			fclose(fp);
			strcpy( cd->trk[0].name, "--" );
			return 0;
		}	                                                                                       			
		else 
		{		

#ifdef DEBUG
			fprintf( stderr,"Warning, can't open \'%s\' \n", fn );
#endif
			strcpy( cd->dtitle, "Unknown / Unknown" );
		
			for( i = cd->first_t; i <= cd->last_t; i++ )
			{
				sprintf( cd->trk[C(i)].name, "Track %d", i );
				cd->trk[C(i)].titled = FALSE;
			}
                        sprintf( fn, "%s%08lx", tcd_dir, cd->cddb_id );
                        tcd_writecddb( cd, fn );
			strcpy( cd->trk[0].name, "--" );
			return 0;
        	}
	}
	fclose(fp);
	return -1;
}

void tcd_writediskinfo( cd_struct *cd )
{
	char fn[60];
	char home[60];

	strcpy( home, getenv("HOME"));
	sprintf( fn, "%s/.tcd/%08lx", home, cd->cddb_id );
	
        if( tcd_writecddb(cd, fn) < 0 )
        {
        	perror( "tcd_writecddb" );
        	exit(-1);
        }

	cd->needs_dbwrite=FALSE;        
	return;
}					

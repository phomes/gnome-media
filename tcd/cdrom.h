/* This file is part of TCD 2.0.
   cdrom.h - Interface independant CDROM routines.

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


#ifndef TCD_CDROM_H
#define TCD_CDROM_H

#define TRK_NAME_LEN 	80
#define DISC_INFO_LEN	80
#define MAXTRACKS	111

#define TRK_PLAYING	0x01 /* 00000001b */
#define TRK_DATA	0x02 /* 00000010b */
#define TRK_REPEAT	0x04 /* 00000100b */ 

struct cd_track
{
	char name[TRK_NAME_LEN+1];
	struct cdrom_tocentry toc;
	int titled;
	int start, length;
	int tot_min, tot_sec;
	int status;
};

typedef struct
{
	int cd_dev;			/* file descriptor */
	char *cdpath;			/* filename of the cdrom dev */
	int cur_t;			/* current track */
	unsigned long old_id; 		/* Unique disc ID */
	unsigned long cddb_id;		/* NEW DISC ID!! */
	struct cd_track trk[MAXTRACKS];	/* Track info, to be allocated 
               			   	   after cd_tchdr is read */

	int first_t, last_t;		/* first and last track numbers
				           1 as the first track. */

	char dtitle[DISC_INFO_LEN+1];	/* Disc title */
	char trkext[MAXTRACKS][DISC_INFO_LEN+1];
	
	/* See /usr/src/linux/include/linux/cdrom.h */
	struct cdrom_ti ti;		/* Track info */
	struct cdrom_tochdr tochdr; 	/* TOC header */
	struct cdrom_subchnl sc;	/* Subchannel, for time */
	int volume;			/* Must range 0-100 */

	int cd_min, cd_sec;		/* Total CD time */
	int t_sec, t_min;		/* Current track time */

	int cur_pos_abs;		/* More timing info */
	int cur_frame;			/* ... */
	int cur_pos_rel;		/* ... */

	int play_method;		/* REPEAT_CD, REPEAT_TRK, NORMAL */
	int repeat_track;		/* Contains the currently repeating
					   track. */

	int cur_disc;			/* For changer use */
					   
	/* Not yet implemented, placeholder for cddb stuff */
	char playlist[80];

	/* error area, these may not all be accurate all the time */
	int isplayable;		/* TRUE if the disc is playable */
	int isdisk;		/* TRUE if there's a disc in the drive */
	int err;		/* TRUE if there's any error */
	int ejected;		/* Internal, used by tcd_ejectcd */
	int needs_dbwrite;	/* Internal */
	char errmsg[100];	/* Human readable error message, filled 
				   if err== TRUE */
	int nslots; 		/* Number of slots the cdrom drive has */
} cd_struct;

/* CD drive control routines */   
void	tcd_opencddev( cd_struct *cd );
int 	tcd_readtoc( cd_struct *cd );
int 	tcd_playtracks( cd_struct *cd, int start_t, int end_t );
int 	tcd_pausecd( cd_struct *cd );
void	tcd_gettime( cd_struct *cd );
int	tcd_readdiskinfo( cd_struct *cd );
void	tcd_writediskinfo( cd_struct *cd );
int 	tcd_ejectcd( cd_struct *cd );
int 	tcd_init_disc( cd_struct *cd );
int 	tcd_stopcd( cd_struct *cd );
int	tcd_close_disc( cd_struct *cd );
int	tcd_change_disc( cd_struct *cd, int disc );	
int 	tcd_play_seconds( cd_struct *cd, long int offset );

/* Some constants */
#define STOPPED 	0
#define PLAYING 	1
#define PAUSED 		2
#define NODISC 		3

#define NORMAL		2
#define REPEAT_CD 	0
#define REPEAT_TRK	1
#define SHUFFLE		3 /* not yet implemented */

#define C(index)	((index)>(MAXTRACKS-1)?0:(index))

#endif /* TCD_CDROM_H */					

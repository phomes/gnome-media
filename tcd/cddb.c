/* This file is part of TCD 2.0.
   cddb.c - CDDB remote and local functions.

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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <pwd.h>
#include <dirent.h>

#include "linux-cdrom.h"

#include "cddb.h"

#define TRUE 1
#define FALSE 0

/* Stuff for the disc_id generation */
struct toc {
        int     min;
	int     sec;
        int     frame;
};
                        
struct toc cdtoc[100];

static int num_digits( int num)
{
	int count = 1;
	
	while ((num/=10) != 0)
		count++;
	return(count);
}

int tcd_readcddb( cd_struct* cd, char* filename )
{
	FILE* fp;
	char string[256];
	int trk;	
	
	fp = fopen( filename, "r" );

	if( fp == NULL )
		return -2;
	if( string == NULL )
		return -1;
	
	/* AC: dont feof.. feof is only true _after_ eof is read */	
	while(fgets( string, 255, fp )!=NULL)
	{
		string[strlen(string)-1] = 0;
		/* If it's a comment, ignore. */
		if( string[0] == '#' )
			continue;		

		/* If it's the disc title, print it */
		if( strncmp( string, "DTITLE", 6 ) == 0)
		{
			strncpy( cd->dtitle, string+7, DISC_INFO_LEN );
			continue;
		}
		if( strncmp( string, "TTITLE", 6 ) == 0 )
		{
                       	if(sscanf( string, "TTITLE%d=", &trk ) == 1)
                        	strncpy( cd->trk[trk+1].name, 
                        	         string + 7 + num_digits(trk),
                        	         TRK_NAME_LEN );
                	else
                		cd->trk[trk+1].name[0] = 0;
		}				
		/* Otherwise ignore it */
	}
	fclose(fp);
	return 0;
}

int tcd_writecddb( cd_struct* cd, char *filename )
{
	FILE *fp;
	int n=0;
	unsigned long i;
	
	fp = fopen( filename, "w" );
	if( fp == NULL )
		return(-1);
					
	fprintf( fp, "# xmcd CD Database Entry\n" );
	fprintf( fp, "#\n" );
	fprintf( fp, "# Track frame offsets:\n" );
	
	/* Print the frame offsets */
	for( i = cd->first_t; i <= cd->last_t; i++ )
	{
		int min, sec;
		
		min = cd->trk[i].toc.cdte_addr.msf.minute;
		sec = cd->trk[i].toc.cdte_addr.msf.second;
	
		n = (min*60)+sec;
		fprintf( fp, "# %u\n", (n*75)+cd->trk[i].toc.cdte_addr.msf.frame );
	}
	/* Print the number of seconds */
	fprintf( fp, "#\n# Disc length: %i seconds\n", 
		(cd->trk[cd->last_t+1].toc.cdte_addr.msf.minute*60)
                +(cd->trk[cd->last_t+1].toc.cdte_addr.msf.second) );

	/* FIXME increment revision. sigh, lousy cddb */
	fprintf( fp, "#\n# Revision: 0\n" );
	fprintf( fp, "# Submitted via: tcd 2.0b\n" );
	fprintf( fp, "#\n" );

	fprintf( fp, "DISCID=%08lx\n", cd->cddb_id );
	fprintf( fp, "DTITLE=%s\n", cd->dtitle );
	for( i = cd->first_t; i <= cd->last_t; i++ )
		fprintf( fp, "TTITLE%ld=%s\n", i-1, cd->trk[i].name );
	fprintf( fp, "PLAYLIST=\n");
	/* FIXME print extended info here too */
	
	fclose(fp);
	return 0;
}

void tcd_call_cddb_slave(cd_struct *cd, char *package, char *version)
{
    char buf[512];
    char tmp[10];
    int blen=512;
    int i, l;
    FILE *fp;
    
    blen-=sprintf( buf, "cddb query %08lx %d ", cd->cddb_id, cd->last_t);
    for( i = cd->first_t; i <= cd->last_t; i++ )
    {
	int min, sec;
	
	min = cd->trk[i].toc.cdte_addr.msf.minute;
	sec = cd->trk[i].toc.cdte_addr.msf.second;
        
	l=sprintf( tmp, "%u ", calc_offset(min,sec,cd->trk[i].toc.cdte_addr.msf.frame));
	
	if(blen>l)
	{
	    strcat( buf, tmp );
	    blen-=l;
	}
    }
    l=sprintf( tmp, "%i\n",
	       (cd->trk[cd->last_t+1].toc.cdte_addr.msf.minute*60)
	       +(cd->trk[cd->last_t+1].toc.cdte_addr.msf.second) );
    if(blen>l)
	strcat( buf,tmp );

    /* ok, buf now holds our query. */
    fflush(stderr);
    fp = popen("cddbslave", "w");
    if(fp == NULL)
    {
	fprintf(stderr, "Slave call failed! (couldn't execute `cddbslave')\n");
	return;
    }

    fprintf(fp, "%s", buf);
    fprintf(fp, "client %s %s %d %d\n", package, version, getpid(), cd->cd_dev);
    pclose(fp);
}


unsigned long cddb_discid( cd_struct *cd )
{
        int     i,
                t = 0,
                n = 0;
	int tot_trks;
	
	tot_trks = cd->last_t;

	for( i=0; i <= cd->last_t+1; i++ )
	{
 		cdtoc[i].frame = cd->trk[i+1].toc.cdte_addr.msf.frame;
 		cdtoc[i].min = cd->trk[i+1].toc.cdte_addr.msf.minute;
 		cdtoc[i].sec = cd->trk[i+1].toc.cdte_addr.msf.second;
 	}
 	

        for (i = 0; i < tot_trks; i++)	{
		n += cddb_sum((cdtoc[i].min * 60) + cdtoc[i].sec);

	        t += ((cdtoc[i+1].min * 60) + cdtoc[i+1].sec) -
       	    		((cdtoc[i].min * 60) + cdtoc[i].sec);
 	}
 	                                            
        return ((n % 0xff) << 24 | t << 8 | (cd->last_t));
}

int cddb_sum(int n)
{
        char    buf[12], *p;
        int     ret = 0;
  
        /* For backward compatibility this algorithm must not change */
        sprintf(buf, "%u", n);
        for (p = buf; *p != '\0'; p++)
                ret += (*p - '0');
        
        return (ret);
}

int calc_offset(int minute, int second, int frame) 
{
	int n;

	n=(minute*60)+second;
	return((n*75)+frame);
}

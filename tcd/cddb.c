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
#include <sys/socket.h>
#include <utsnamelen.h>


#ifdef linux
# include <linux/cdrom.h>
# include "linux-cdrom.h"
#else
# error TCD currently only builds under Linux systems.
#endif

#include "socket.h"
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
                        

int tcd_readcddb( cd_struct* cd, char* filename )
{
	FILE* fp;
	char string[100];
	char num[80];
	int trk;	
	
	fp = fopen( filename, "r" );

	if( fp == NULL )
		return -2;
	if( string == NULL )
		return -1;
	
	/* AC: dont feof.. feof is only true _after_ eof is read */	
	while(fgets( string, 80, fp )!=NULL)
	{
		string[strlen(string)-1] = 0;
		// If it's a comment, ignore.
		if( string[0] == '#' )
			continue;		

		// If it's the disc title, print it
		if( strncmp( string, "DTITLE", 6 ) == 0)
		{
			strncpy( cd->dtitle, string+7, DISC_INFO_LEN );
			continue;
		}
		if( strncmp( string, "PLAYLIST", 8 ) == 0)
		{
			strncpy( cd->playlist, string+9, 80 );
			continue;
		}
		if( strncmp( string, "TTITLE", 6 ) == 0 )
		{
                       	if(sscanf( string, "TTITLE%d=%[ -z]", &trk, num ) == 2)
                        	strncpy( cd->trk[trk+1].name, num, TRK_NAME_LEN );
                	else
                		cd->trk[trk+1].name[0] = 0;
		}				
		// Otherwise ignore it
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
					
	fprintf( fp, "# xmcd CD Database Entry\n" );		// Signature
	fprintf( fp, "#\n" );			// Blank
	fprintf( fp, "# Track frame offsets:\n" );
	
	// Print the frame offsets
	for( i = cd->first_t; i <= cd->last_t; i++ )
	{
		int min, sec;
		
		min = cd->trk[i].toc.cdte_addr.msf.minute;
		sec = cd->trk[i].toc.cdte_addr.msf.second;
	
		n = (min*60)+sec;
		fprintf( fp, "# %u\n", (n*75)+cd->trk[i].toc.cdte_addr.msf.frame );
	}
	// Print the number of seconds
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
	for( i = cd->first_t; i <= cd->last_t; i++ )
		fprintf( fp, "EXTT%ld=%s\n", i-1, cd->trkext[i] );
	fprintf( fp, "PLAYLIST=%s\n", cd->playlist );
	/* FIXME print extended info here too */
	
	fclose(fp);
	return 0;
}

int tcd_sendhandshake( 	cddb_server *server,
			char* username, 
			char* hostname, 
			char* clientname, 
			char* version )
{
	char tmp[512];
	
	/* length sanity - AC */
	snprintf( tmp, 512, "cddb hello %s %s %s %s\n", username,
						  hostname,
						  clientname,
						  version );
						
	send( server->socket, tmp, strlen(tmp), 0 );
	return 0;
}

/*
 *	Corrupt reply - gets you ???? 
 */
 
static char *if_strtok(char *p, const char *p2)
{
	p=strtok(p,p2);
	if(p==NULL)
		return("????");
	return p;
}

int tcd_getquery( cddb_server *server, cddb_query_str *query )
{
	char s[512];
	fgetsock( s, 511, server->socket );
	s[511]=0;
	
	/* Fill in query info  - watch lengths - AC */
	query->code = atoi(strtok( s, " " ));
	strncpy(query->categ, if_strtok(NULL, " ") , CATEG_MAX);
	query->categ[CATEG_MAX-1]=0;
	strncpy(query->discid, if_strtok( NULL, " "), DISCID_MAX);
	query->discid[DISCID_MAX-1]=0;
	strncpy(query->dtitle, if_strtok( NULL, "\0"), DTITLE_MAX);
	query->dtitle[DTITLE_MAX-1]=0;
	return 0;
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

/*
 *	The caller passes us a buffer length constraint now - AC
 */
 
void tcd_formatquery( cd_struct *cd, char *buf , int blen)
{
	char tmp[10];
	int i,n,l;
		
	sprintf( buf, "cddb query %08lx %d ", cd->cddb_id, cd->last_t);
	for( i = cd->first_t; i <= cd->last_t; i++ )
	{
		int min, sec;
		
		min = cd->trk[i].toc.cdte_addr.msf.minute;
		sec = cd->trk[i].toc.cdte_addr.msf.second;
	
		n = (min*60)+sec;
		l=sprintf( tmp, "%u ", (n*75)+cd->trk[i].toc.cdte_addr.msf.frame );
		if(blen>=l)
		{
			strcat( buf, tmp );
			blen-=l;
		}
	}
        l=sprintf( tmp, "%i\n",
                        (cd->trk[cd->last_t+1].toc.cdte_addr.msf.minute*60)
                        +(cd->trk[cd->last_t+1].toc.cdte_addr.msf.second) );
        if(blen>=l)
		strcat( buf,tmp );                                                                               
}

int tcd_open_cddb( cddb_server *server )
{
	struct passwd* pw;
	int code;
	char s[200];
	
	/* FIXME - is there a 'right' way to set this portably */
	char hostname[65];
	
	gethostname(hostname, 65);
	if( strcmp(hostname, "") == 0)
		strcpy( hostname, "generic" );
	pw = getpwuid(getuid());
	/* Password entry gone - eg crashed NIS servers */
	if(pw==NULL)
	{
		strcpy(server->error, "Your username isnt available." );
		return -1;
	}
	if( strcmp(pw->pw_name, "") == 0)
		strcpy( pw->pw_name, "user" );

	server->socket = opensocket( server->hostname, server->port );
	if( server->socket < 0 )
	{
		strcpy( server->error, strerror(errno) );
		return -1;
	}
	fgetsock( s, 80, server->socket );
	if( sscanf( s, "%d", &code ) != 1 )
	{
		strcpy( server->error, "CDDB Server returned garbled information." );
		return -1;
	}
	if( code != 200 || code != 201 )
	{
		switch( code )
		{
			case 432:
				strcpy( server->error, 
					"Permission Denied" );
				return -1;		
			case 433:
				strcpy( server->error, 
					"Too many users" );
				return -1;		
			case 434:
				strcpy( server->error, 
					"Load too high" );
				return -1;		
			default:
				strcpy( server->error,
					"Unknown error" );
		}
	}

	tcd_sendhandshake( server,pw->pw_name,
					hostname,
			        	"TCD",
			        	"2.0" );

	fgetsock( s, 80, server->socket );
	if( sscanf( s, "%d", &code ) != 1 )
	{
		strcpy( server->error, "CDDB Server returned garbled information." );
		return -1;
	}
	if( code != 200 || code != 201 )
	{
		switch( code )
		{
			case 431:
				strcpy( server->error, 
					"Handshake not successful" );
				return -1;		
			case 433:
				strcpy( server->error, 
					"Already shook hands (SHOULD NOT HAPPEN!)" );
				return -1;		
			default:
				strcpy( server->error,
					"Unknown error" );
		}
	}
	return 0;
}

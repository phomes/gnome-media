/* This file is part of TCD 2.0.
   cddb.h - Header file for CDDB routines.

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


#ifndef TCD_CDDB_H
#define TCD_CDDB_H

#define HOSTNAME_MAX	50
#define VERSION_MAX	10
#define CATEG_MAX	20	
#define DISCID_MAX	10
#define DTITLE_MAX	80

typedef struct {
	char hostname[HOSTNAME_MAX];
	int port;
	int socket;
	int connected;
	char error[100];
} cddb_server;

typedef struct {
	int code;
	char categ[CATEG_MAX];
	char discid[DISCID_MAX];
	char dtitle[DTITLE_MAX];
} cddb_query_str;
	

int 		tcd_readcddb( cd_struct* cd, char* filename );
int 		tcd_writecddb( cd_struct* cd, char *filename );
int 		cddb_sum(int n);
unsigned long 	cddb_discid( cd_struct *cd );
int 		tcd_sendhandshake(  cddb_server *server,
                		        char* username,
                			char* hostname,
                		        char* clientname,
                		        char* version );
int 		tcd_getquery( cddb_server *server, cddb_query_str *query );
void 		tcd_formatquery( cd_struct *cd, char *buf , int blen);
int 		tcd_open_cddb( cddb_server *server );


#endif /* TCD_CDDB_H */

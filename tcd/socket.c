/* This file is part of TCD 2.0.
   socket.c - Socket functions for cddb.c.

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
                                                
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <stdarg.h>
#include <sys/time.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "socket.h"

int opensocket( char *hostname, int port )
{
	int sockfd;
        struct hostent *he;
        struct sockaddr_in their_addr; /* connector's address information */
        
        he = malloc( sizeof( struct hostent ) );
        
        if ((he=gethostbyname(hostname)) == NULL) {  /* get the host info */
        	return -1;
	}
                                                        
        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        	return -1;
	}
        their_addr.sin_family = AF_INET;      /* host byte order */
        their_addr.sin_port = htons(port);    /* short, network byte order */
        their_addr.sin_addr = *((struct in_addr *)he->h_addr);
        bzero(&(their_addr.sin_zero), 8);     /* zero the rest of the struct */
        
        if (connect(sockfd, (struct sockaddr *)&their_addr, \
	  		sizeof(struct sockaddr)) == -1) {
		return -1;
        }
	
	return sockfd;
}

int fgetsock(char* s, int size, int socket, PeriodicFunc func)
{
	fd_set rfds;
	struct timeval tv;
	int retval;
	int i=0, r;
        char c;

	FD_ZERO(&rfds);
	FD_SET(socket, &rfds);
	tv.tv_sec = 0;
	tv.tv_usec = 10;

	retval = select(socket+1, &rfds, NULL, NULL, &tv);
	
	if(!retval)
		if(func) func();
	        
        while( (r=recv(socket, &c, 1, 0)) != 00 )
        {
        	if(c == '\r' || c == '\n')
                	break;
		if( i > size )
                	break;
                s[i] = c;
		i++;
		if(func) func();
	}
        s[i] = 0;
	recv(socket, &c, 1, 0);
	if( r < 0 )
		return -1;
	return r;
}

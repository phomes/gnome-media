#define PACKAGE "TCD"                                             
#define VERSION "2.0"                                             

/* This file is part of TCD 2.0.
   tcd.c - Main source file for curses interface.
   
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
   

#include <fcntl.h>
#include <curses.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <math.h>
#include <time.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>

#ifdef linux
# include <linux/cdrom.h>
# include "linux-cdrom.h"
#else
# error TCD currently only builds under Linux systems.
#endif

#include "tcd.h"
#include "tracked.h"
#include "cddb.h"
#include "socket.h"

WINDOW *win;
int maxx, maxy;
cd_struct* cd;

void tcd_get_cddb( void );

char *play_methods[] =
{
        "Repeat CD",
	"Repeat Track",
        "Normal",
        "Random"
};
                                

int main( int argc, char **argv )
{
	int quit=FALSE, track, i;
	char c;
	time_t t1, t2;
	
	/* Install signal handler */
	for( i=SIGHUP; i<SIGTERM+1; i++ )
		signal( i, sighandler );

	/* Allocate some memory for the cd structure */
	cd = malloc(sizeof(cd_struct));
	memset( cd, 0, sizeof(cd_struct) );
	cd->play_method = NORMAL;

	init_curses();

	if( maxx < 80 || maxy < 24 )
	{
		endwin();
		printf( "Error: screen size too small, needs 80x25, has %dx%d.\n", 
			maxx, maxy ); 
		exit(-1);
	}

	cd->cdpath = malloc( 50 );
	
        if( argc < 2 )
		strcpy( cd->cdpath, "/dev/cdrom" );
	else
        	strcpy( cd->cdpath, argv[1] );
		
	tcd_init_disc(cd, NULL);
	draw_not_always();
        t1 = time(NULL);
        t2 = t1;
	cd->needs_dbwrite=FALSE;
	
	while( !quit )
	{		
		
		/* if the user hasn't stopped the cd, but it is 
		   stopped anyway, fix it. */
		if( cd->sc.cdsc_audiostatus != CDROM_AUDIO_PLAY &&
		    cd->sc.cdsc_audiostatus != CDROM_AUDIO_PAUSED )
		{
			if( cd->play_method == REPEAT_CD )
				tcd_playtracks( cd, cd->first_t, cd->last_t );
		}				                                                        
		if( cd->play_method==REPEAT_TRK && cd->cur_t != cd->repeat_track )
			tcd_playtracks( cd, cd->repeat_track, cd->last_t );
		
		if( cd->isplayable ) tcd_gettime(cd);

		update_display();
		wmove( win, maxy-1,maxx-1 );
		c = wgetch(win); 

		switch( c )
		{
			case 'u':
			case 'U':
				if( cd->isplayable ) tcd_pausecd(cd);
				break;
			case 'p':
			case 'P':
				if( cd->isdisk ) {
					if( cd->sc.cdsc_audiostatus == CDROM_AUDIO_PAUSED )
						tcd_pausecd(cd);
					else
						tcd_playtracks( cd,cd->first_t,cd->last_t );
					update_tracklist();
				}
				break;
			case 'q':
			case 'Q':
				quit = TRUE;
				break;
			case '=':
			case '+':
				if( (cd->cur_t < cd->last_t) && cd->isplayable ) {	
					cd->cur_t++;
					tcd_playtracks( cd,cd->cur_t, cd->last_t );
					if( cd->play_method==REPEAT_TRK )
						cd->repeat_track = cd->cur_t;
				}
				break;
			case '-':
			case '_':
				if( (cd->cur_t > cd->first_t) && cd->isplayable ) {
					if( cd->t_sec < 10 )
						cd->cur_t--;
			        	tcd_playtracks( cd,cd->cur_t, cd->last_t );
					if( cd->play_method==REPEAT_TRK )
						cd->repeat_track = cd->cur_t;
				}
			       	break;                                                                                                         
			case 'g':
			case 'G':
				if( cd->isdisk ) {
					if( (track = get_i_track())!=-1 )
					{
						cd->cur_t = track;
						tcd_playtracks( cd, track, cd->last_t );
						if( cd->play_method==REPEAT_TRK )
							cd->repeat_track = cd->cur_t;
					}
				}
				break;
			case 'c':
			case 'C':
				cd->play_method = REPEAT_CD;
				break;
			case 'r':
			case 'R':
				cd->play_method = REPEAT_TRK;
				cd->repeat_track = cd->cur_t;
				break;
			case 'm':
			case 'M':
				cd->play_method++;
				if( cd->play_method > NORMAL )
					cd->play_method = REPEAT_CD;
				if( cd->play_method < REPEAT_CD )
					cd->play_method = NORMAL;

				if( cd->play_method == REPEAT_TRK )
					cd->repeat_track = cd->cur_t;
				else
					cd->repeat_track = -1;
				
				break;
			case 'e':
			case 'E':
				tcd_ejectcd(cd);
				cd->play_method = NORMAL;
				cd->repeat_track = -1;
				break;
			case 't':
			case 'T':
				if( cd->isplayable )
					edit_trackdb();
				draw_not_always();
				break;
			case 's':
			case 'S':
				if( cd->isplayable )
					tcd_stopcd(cd);
				break;				
			case '<':
			case ',':
				cd->volume -= 4;
				if( cd->volume < 0 )
					cd->volume = 0;
				break;
			case '>':
			case '.':
				cd->volume += 4;
				if( cd->volume > 255 )
					cd->volume = 255;
				break;
#ifdef TCD_CHANGER_ENABLED
			case '1':
			case '2':
			case '3':
			case '4':
				tcd_close_disc(cd);
				tcd_change_disc( cd, c-49 );
				tcd_init_disc(cd,NULL);

			        wclear(win);
		                wrefresh(win);
				draw_not_always();
				cd->play_method = NORMAL;
				break;
#endif
			case 'd':
			case 'D':
				tcd_get_cddb();
				tcd_close_disc(cd);
				
				free(cd->cdpath);
				free(cd);
				cd = malloc( sizeof(cd_struct) );
				cd->cdpath = malloc( 50 );	
			        if( argc < 2 )
					strcpy( cd->cdpath, "/dev/cdrom" );
				else
        				strcpy( cd->cdpath, argv[1] );

				cd->play_method = NORMAL;
				tcd_init_disc(cd, NULL);
			        wclear(win);
		                wrefresh(win);
				draw_not_always();
				break;
			case ']':
				tcd_play_seconds(cd, 4);
				cd->repeat_track = cd->cur_t;
				break;
			case '[':
				tcd_play_seconds(cd, -4);
				cd->repeat_track = cd->cur_t;
				break;
			case ERR:
				wrefresh(win);
				update_display();
			default:
				break;
		}
		t2=time(NULL);
		if( t1 < t2 )
		{
			if( cd->err || !cd->isdisk )
			{
             			tcd_readtoc(cd);
 			        if( !cd->err )
                        	{
                                	tcd_readdiskinfo(cd);
                                        cd->err = FALSE;
                                        cd->isdisk = TRUE;
                                }                         
			        wclear(win);
		                wrefresh(win);
				draw_not_always();
			}
		        t1=time(NULL);
		}
	}			

	wclear(win);
	wrefresh(win);
	wmove(win,0,0);
	endwin();

	if( cd->needs_dbwrite )
		tcd_writediskinfo(cd);

	tcd_close_disc( cd );
	free(cd->cdpath);
	free(cd);
	
	return(0);
}

int init_curses( void )
{
	win = initscr();

	if( has_colors() )
		start_color();
	
	init_colors();		
	
	cbreak();
	noecho();
	leaveok( win, TRUE );

	getmaxyx( win, maxy, maxx );

	print_help();
	halfdelay( 5 );
	return(0);
}

void update_display( void )
{
	char tmp[TRK_NAME_LEN];
	int i;
	
	attron(C_YELLOW+A_BOLD);
	mvwprintw(win,7,30, "[M]" );
	attroff(C_YELLOW+A_BOLD);
	
	mvwprintw(win,7,33, " - Change play mode." );
	mvwprintw(win,8,30, "Current Mode:             " );
	
	attron(C_RED+A_BOLD);
	mvwprintw(win,8,44, "%s", play_methods[cd->play_method] );
	attroff(C_RED+A_BOLD);

	mvwprintw(win,8,56, "Current Volume:        " );
	
	attron(C_RED+A_BOLD);
	mvwprintw(win,8,71, " %d%%", (int)floor(cd->volume/2.55) );
	attroff(C_RED+A_BOLD);
	
	mvwprintw(win,3,2, "Track:  %d of %d    ", 
		cd->cur_t,cd->last_t );
	mvwprintw(win,4,2, "Time:  %c%02u:%02u    ", 
		(cd->cur_pos_rel<0)?'-':' ',
		cd->t_min,cd->t_sec );

	mvwprintw(win,5,2, "CD:     %02u:%02u    ",
		cd->cd_min,cd->cd_sec );
	
	/* Clear the track area */
	for( i = 38; i < maxx-1; i++ )
		mvwprintw(win,maxy-2,i," " );

	/* Make sure the track name isn't too long */
	if( strlen(cd->trk[C(cd->cur_t)].name) > maxx-39)
	{
		strcpy( tmp, cd->trk[C(cd->cur_t)].name );
		tmp[maxx-39] = 0;
		tmp[maxx-38] = '.';
		tmp[maxx-37] = '.';
		tmp[maxx-36] = '.';
	}
	else
		strcpy( tmp, cd->trk[C(cd->cur_t)].name );
		
        attron( C_RED+A_BOLD );
        mvwprintw(win,maxy-2,38,"%s", tmp );
	attroff( C_RED+A_BOLD );


#ifdef TCD_CHANGER_ENABLED

	attron( C_BLUE + A_BOLD );
	strcpy( tmp, "" );
	for( i=0; i < cd->nslots; i++ )
	{
		if( cd->cur_disc == i )
			strcat( tmp, "[X] " );
		else
			strcat( tmp, "[ ] " );
	}
	
	mvwprintw( win, maxy-5, 30, "Disc Information - %s", tmp );
	attroff( C_BLUE+A_BOLD );
#endif

	if( !cd->err ) update_tracklist();
	update_status();
}

void draw_not_always()
{	
	print_help();

	attron( C_BLUE );
	box( win, ACS_VLINE,ACS_HLINE );
	wmove( win, 0, 28 );		wvline( win, ACS_VLINE, maxy-1 );
	wmove( win, maxy-5, 29 );	whline( win, ACS_HLINE, maxx-31 );
	wmove( win, 7, 1 );		whline( win, ACS_HLINE, 27 );
	wmove( win, 0, 28 );		whline( win, ACS_ULCORNER, 1 );
	attroff( C_BLUE );

	attron( C_BLUE + A_BOLD );
	mvwprintw( win, 0, 30, 	"Control Panel" );

	mvwprintw( win, maxy-5, 30, "Disc Information" );
	mvwprintw( win, 7, 2, "Track List" );
	attroff( C_BLUE+A_BOLD );
	
	attron(C_WHITE+A_BOLD);
	mvwprintw(win,0,2, "TCD v2.0, by Tim Gerla" );	
	attroff(C_WHITE+A_BOLD);
	
        mvwprintw(win,maxy-4,30,"Length:" );
        mvwprintw(win,maxy-3,30,"Title:" );
        mvwprintw(win,maxy-2,30,"Track:" );
	attron( C_RED+A_BOLD );
        mvwprintw(win,maxy-4,38,"%02u:%02u",
		cd->trk[C(cd->last_t+1)].toc.cdte_addr.msf.minute,
	        cd->trk[C(cd->last_t+1)].toc.cdte_addr.msf.second );
        mvwprintw(win,maxy-3,38,cd->dtitle );
 	attroff( C_RED+A_BOLD );
}

void print_help()
{
	phelp( 2, 30, 'P',"- Start playing.         ",NOTOG,cd->isplayable );
	phelp( 3, 30, 'U',"- Pause or restart.      ",NOTOG,cd->isplayable );
	phelp( 4, 30, 'E',"- Eject CDROM.           ",NOTOG,1 );
	phelp( 5, 30, 'S',"- Stop playing.          ",NOTOG,cd->isplayable );

	phelp( 2, 56, '-',"- Previous track.  ",NOTOG,cd->isplayable );
	phelp( 3, 56, '+',"- Next track.      ",NOTOG,cd->isplayable );
	phelp( 4, 56, 'G',"- Go to track #.   ",NOTOG,cd->isplayable );
	phelp( 5, 56, ']',"- Skip ahead.   ",NOTOG,cd->isplayable );
	phelp( 6, 56, '[',"- Skip back.   ",NOTOG,cd->isplayable );

	phelp( 14,30, 'T',"- Edit track database.   ",NOTOG,cd->isplayable );
	phelp( 15,30, 'D',"- Download CDDB data.   ",NOTOG,cd->isplayable );
	
	attron(C_YELLOW+A_BOLD);
	mvwprintw( win, 7, 56, "< >" );
	attroff(C_YELLOW+A_BOLD);
	mvwprintw( win, 7, 59, " - Adjust volume." );

	phelp( 18,30, 'Q',"- Quit.                  ",NOTOG,1 );

/* Not yet implemented 
 *
 *	phelp( 2, 57, '[',"- Lower volume.",NOTOG,cd->isplayable );
 *	phelp( 3, 57, ']',"- Raise volume.",NOTOG,cd->isplayable );
 *	phelp( 6, 57, '<',"- Rewind.",NOTOG,cd->isplayable );
 *	phelp( 7, 57, '>',"- Fastforward.", NOTOG,cd->isplayable );
 *
 */
}	

void update_tracklist( void )
{
	int height=9;
	char stat;
 	int i,j,x=0,y=0;

	j=height;
	if( cd->isdisk )
	{
		for( i=cd->first_t; i <= cd->last_t; i++ ) {
			switch(cd->trk[C(i)].status) {
			case TRK_PLAYING:
				if( cd->sc.cdsc_audiostatus == CDROM_AUDIO_PLAY ) 
					stat = 'P';
				else 
					stat = 'a';
				break;
			case TRK_REPEAT:
				stat = 'R';
				break;
			case TRK_DATA:
				stat = 'd';
				break;
			default:
				stat = 'a';
				break;
			}
			 
		
			if( (i+height) > maxy-2 ) {	
				y = j++;	
				x = 16;
			}
			else {
				y = (height-1)+i;
				x = 2;
	 		}
			
			if( cd->sc.cdsc_audiostatus == CDROM_AUDIO_PLAY && cd->cur_t == i )
				attron( A_BOLD );
				
			mvwprintw(win,y,x,"%02u%c - %02u:%02u",
				i, stat,cd->trk[C(i)].tot_min,
				cd->trk[C(i)].tot_sec );

			if( cd->sc.cdsc_audiostatus == CDROM_AUDIO_PLAY && cd->cur_t == i )
				attroff( A_BOLD );
		}	
	}
	else
	{
		for( i = height; i < maxy-2; i++ )
			mvwprintw( win, i, 2, "                         " );
	}
}

void update_status( void )
{
	char tmp[80];
	
	if( !cd->err )
	{
		switch( cd->sc.cdsc_audiostatus )
		{
		case CDROM_AUDIO_INVALID:
			strcpy( tmp,"No Audio         " );
			break;
		case CDROM_AUDIO_PLAY:
			strcpy( tmp,"Playing          " );
			break;
		case CDROM_AUDIO_PAUSED:
			strcpy( tmp,"Paused           " );
			break;
		case CDROM_AUDIO_COMPLETED:
			strcpy( tmp,"Stopped          " );
			break;
		case CDROM_AUDIO_ERROR:
			strcpy( tmp,"Error            " );
			break;
		case CDROM_AUDIO_NO_STATUS:
			strcpy( tmp,"Stopped          " );
			break;
		default:
			strcpy( tmp,"                 " );
		}
	}
	else strcpy( tmp, cd->errmsg );
	
	mvwprintw( win, 2,2, "Status:" );
	attron( C_RED+A_BOLD );
	mvwprintw( win, 2,10, tmp );
	attroff( C_RED+A_BOLD );
}

int get_i_track()
{
	char tmp[5];
	int track;
	
	cbreak();
	echo();
	leaveok( win, FALSE );

	mvwprintw( win,3,2,"Go to:    /%02u       ",cd->last_t );
	wmove( win, 3,10 );
	wgetnstr(win, tmp, 2);
	nocbreak();
	noecho();
	halfdelay(1);

	track = atoi(tmp);
	
	if( track < cd->first_t || track > cd->last_t )
		track=-1;

	leaveok( win, TRUE );
	return( track );
}

void init_colors( void )
{
	if( !has_colors() )
		return;
		
	init_pair( 0, COLOR_BLACK, COLOR_BLACK );
	init_pair( 1, COLOR_RED, COLOR_BLACK );
	init_pair( 2, COLOR_GREEN, COLOR_BLACK );
	init_pair( 3, COLOR_YELLOW, COLOR_BLACK );
	init_pair( 4, COLOR_BLUE, COLOR_BLACK );
	init_pair( 5, COLOR_MAGENTA, COLOR_BLACK );
	init_pair( 6, COLOR_CYAN, COLOR_BLACK );
	init_pair( 7, COLOR_WHITE, COLOR_BLACK );
}	

void phelp( int y, int x, char key, char *func, int toggle, int enabled )
{
	mvwprintw( win, y,x+4, func );

	attron( enabled?C_YELLOW+A_BOLD:A_DIM );
	mvwprintw( win, y,x,"[%c]", key );
	attroff( enabled?C_YELLOW+A_BOLD:A_DIM );
	
	if( toggle == NOTOG )
	{
		attron( enabled?C_YELLOW+A_BOLD:A_DIM );
		mvwprintw( win, y,x,"[%c]", key );
		attroff( enabled?C_YELLOW+A_BOLD:A_DIM );
		return;
	}	
	if( toggle && enabled )
	{
		attron( A_REVERSE+A_NORMAL );
		mvwprintw( win, y,x,"[%c]", key );
		attroff( A_REVERSE+A_NORMAL );
		return;
	}
	if( toggle && !enabled )
	{
		attron( A_REVERSE+A_DIM );
		mvwprintw( win, y,x,"[%c]", key );
		attroff( A_REVERSE+A_DIM );
		return;
	}
	if( !toggle && enabled )
	{
		attron( A_BOLD+C_YELLOW );
                mvwprintw( win, y,x,"[%c]", key );
		attroff( A_BOLD+C_YELLOW );
		return;
	}
	if( !toggle && !enabled	)
	{
		attron( A_DIM );
		mvwprintw( win, y,x,"[%c]", key );
		attroff( A_DIM );
		return;
	}		                                                        
		
	return;
}

void tcd_get_cddb( void )
{
	cddb_server server;
	int i,r;
	char s[80], test[200];
	FILE *outfile;
	cddb_query_str query;
	
	strcpy( server.hostname, "cddb.cddb.com" );
	server.port = 888;
	                
	mvwprintw( win, maxy-1, 2, "Connecting to cddb.cddb.com:888...   " );refresh();	
	if( tcd_open_cddb( &server, NULL ) != 0 )
	{
		mvwprintw( win, maxy-1, 2, "Error: %s                    ", server.error );refresh();	
		sleep(2);
		return;
	}
	mvwprintw( win, maxy-1, 2, "Connected. Searching for album.      " );refresh();	

	tcd_formatquery( cd, test ,sizeof(test));
        r = send( server.socket, test, strlen(test), 0 );
	
        tcd_getquery( &server, &query, NULL );
	
	mvwprintw( win, maxy-1, 2, "Album found. Downloading information." );refresh();	
 
        sprintf( test, "cddb read %s %s\n", query.categ, query.discid );
 	send( server.socket, test, strlen(test), 0 );

	sprintf( test, "%s/.tcd/%s", getenv("HOME"),query.discid );
	outfile = fopen( test, "w" );

	i=0;
	do
	{
		fgetsock( s, 80, server.socket,NULL );
	        fprintf( outfile, "%s\n", s );
		mvwprintw( win, maxy-1, 2, "Downloading line %3d...             ",i++ );refresh();
	} while( strcmp( ".", s ) );

	fclose(outfile);
	close(server.socket);
	                                                                                 	                                                                   	
}        	                                                                          

void sighandler( int sig )
{ 
	close(cd->cd_dev);	
	wclear(win);
	wrefresh(win);
	wmove(win,0,0);
	endwin();
	
	printf( "Signal %d caught, exiting.\n", sig );
	printf( "Eep! Please mail any info (no core dumps please.) to timg@means.net!\n\n" );
	
	exit(sig);
} 

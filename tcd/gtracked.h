/* This file is part of TCD 2.0.
   gtracked.h - Headers and data structures for the GTK track editor.

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



#ifndef GTRACKED_H
#define GTRACKED_H

struct pass_str
{
	cd_struct *cd;
	GtkWidget *window;	
};

void edit_window( void );
void destroy_window (GtkWidget *widget, GtkWidget **window);
void gtracked();

#endif /* GTRACKED_H */

/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 *  Authors: Iain Holmes <iain@ximian.com>
 *
 *  Copyright 2002 Ximain, Inc. (www.ximian.com)
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Street #330, Boston, MA 02111-1307, USA.
 *
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <libgnome/gnome-util.h>

#include <libxml/tree.h>
#include <libxml/parser.h>

#include "gnome-cd.h"

static char *
make_fullname (const char *theme_name,
	       const char *name)
{
	char *loc, *image;

	loc = g_concat_dir_and_file (theme_name, name);
	image = g_concat_dir_and_file (THEME_DIR, loc);
	g_free (loc);

	return image;
}

static void
parse_theme (GCDTheme *theme,
	     xmlDocPtr doc,
	     xmlNodePtr cur)
{
	while (cur != NULL) {
		if (xmlStrcmp (cur->name, (const xmlChar *) "image") == 0) {
			xmlChar *button;
			
			button = xmlGetProp (cur, (const xmlChar *) "button");
			if (button != NULL) {
				char *file, *full;

				file = xmlNodeListGetString (doc, cur->xmlChildrenNode, 1);
				full = make_fullname (theme->name, file);

				if (xmlStrcmp (button, "previous") == 0) {
					theme->previous = gdk_pixbuf_new_from_file (full, NULL);
				} else if (xmlStrcmp (button, "rewind") == 0) {
					theme->rewind = gdk_pixbuf_new_from_file (full, NULL);
				} else if (xmlStrcmp (button, "play") == 0) {
					theme->play = gdk_pixbuf_new_from_file (full, NULL);
				} else if (xmlStrcmp (button, "pause") == 0) {
					theme->pause = gdk_pixbuf_new_from_file (full, NULL);
				} else if (xmlStrcmp (button, "stop") == 0) {
					theme->stop = gdk_pixbuf_new_from_file (full, NULL);
				} else if (xmlStrcmp (button, "forward") == 0) {
					theme->forward = gdk_pixbuf_new_from_file (full, NULL);
				} else if (xmlStrcmp (button, "next") == 0) {
					theme->next = gdk_pixbuf_new_from_file (full, NULL);
				} else if (xmlStrcmp (button, "eject") == 0) {
					theme->eject = gdk_pixbuf_new_from_file (full, NULL);
				} else {
					/* Hmmm */
				}

				g_free (full);
			} else {
				/* Check for menu */
				char *menu;

				menu = xmlGetProp (cur, (const xmlChar *) "menu");
				if (menu != NULL) {
					char *file, *full;

					file = xmlNodeListGetString (doc, cur->xmlChildrenNode, 1);
					full = make_fullname (theme->name, file);
					
					if (xmlStrcmp (menu, "previous") == 0) {
						theme->previous_menu = gdk_pixbuf_new_from_file (full, NULL);
					} else if (xmlStrcmp (menu, "stop") == 0) {
						theme->stop_menu = gdk_pixbuf_new_from_file (full, NULL);
					} else if (xmlStrcmp (menu, "play") == 0) {
						theme->play_menu = gdk_pixbuf_new_from_file (full, NULL);
					} else if (xmlStrcmp (menu, "next") == 0) {
						theme->play_menu = gdk_pixbuf_new_from_file (full, NULL);
					} else if (xmlStrcmp (menu, "eject") == 0) {
						theme->eject_menu = gdk_pixbuf_new_from_file (full, NULL);
					} else {
						/* Hmm */
					}

					g_free (full);
				}
			}
		}

		cur = cur->next;
	}
}
				
GCDTheme *
theme_load (GnomeCD *gcd,
	    const char *theme_name)
{
	char *theme_path, *xml_file, *tmp;
	GCDTheme *theme;
	xmlDocPtr xml;
	xmlNodePtr ptr;
	
	g_return_val_if_fail (gcd != NULL, NULL);
	g_return_val_if_fail (theme_name != NULL, NULL);

	g_print ("HEllo theme world\n");
	theme_path = g_concat_dir_and_file (THEME_DIR, theme_name);
	if (g_file_test (theme_path, G_FILE_TEST_IS_DIR) == FALSE) {
		/* Theme dir isn't a dir */

		g_print ("Not a dir %s\n", theme_path);
		g_free (theme_path);
		return NULL;
	}

	tmp = g_strconcat (theme_name, ".theme", NULL);
	xml_file = g_concat_dir_and_file (theme_path, tmp);
	g_free (tmp);

	if (g_file_test (xml_file,
			 G_FILE_TEST_IS_REGULAR |
			 G_FILE_TEST_IS_SYMLINK) == FALSE) {
		/* No .theme file */

		g_print ("No .theme file: %s\n", xml_file);
		g_free (theme_path);
		g_free (xml_file);
		return NULL;
	}

	g_print ("Parsin file\n");
	xml = xmlParseFile (xml_file);
	g_free (xml_file);
	
	if (xml == NULL) {

		g_print ("No XML\n");
		g_free (theme_path);
		return NULL;
	}

	/* Check doc is right type */
	ptr = xmlDocGetRootElement (xml);
	if (ptr == NULL) {

		g_print ("No root\n");
		g_free (theme_path);
		xmlFreeDoc (xml);
		return NULL;
	}
	if (xmlStrcmp (ptr->name, "gnome-cd")) {
		g_print ("Not gnome-cd: %s\n", ptr->name);
		g_free (theme_path);
		xmlFreeDoc (xml);
		return NULL;
	}
	
	theme = g_new (GCDTheme, 1);

	theme->name = g_strdup (theme_name);
	/* Walk the tree filing in the values */
	ptr = ptr->xmlChildrenNode;
	while (ptr && xmlIsBlankNode (ptr)) {
		ptr = ptr->next;
	}

	if (ptr == NULL) {
		g_print ("Empty theme\n");
		return NULL;
	}

	while (ptr != NULL) {
		if (xmlStrcmp (ptr->name, (const xmlChar *) "display") == 0) {
			g_print ("Parsing display theme\n");
			cd_display_parse_theme (gcd->display, theme, xml, ptr->xmlChildrenNode);
		} else if (xmlStrcmp (ptr->name, (const xmlChar *) "icons") == 0) {
			g_print ("Parsing widget theme\n");
			parse_theme (theme, xml, ptr->xmlChildrenNode);
		} else {
			g_print ("Uknown element: %s\n", ptr->name);
		}

		ptr = ptr->next;
	}

	return theme;
}

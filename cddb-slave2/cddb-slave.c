/*
 * cddb-slave.c: Implementation for the GNOME/Media/CDDBSlave2 interface.
 *
 * Copyright (C) 2001 Ximian, Inc.
 * 
 * Authors: Iain Holmes  <iain@ximian.com>
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <bonobo/bonobo-object.h>
#include <bonobo/bonobo-event-source.h>
#include <bonobo/bonobo-main.h>
#include <bonobo/bonobo-arg.h>
#include <bonobo/bonobo-exception.h>
#include <gnome.h>

#include <gconf/gconf-client.h>

/* Use local copy of gnet */
#include "gnet.h"

#include <orbit/orbit.h>

#include "cddb-slave.h"
#include "GNOME_Media_CDDBSlave2.h"


static GConfClient *client = NULL;

#define PARENT_TYPE BONOBO_OBJECT_TYPE
static BonoboObjectClass *parent_class = NULL;

typedef enum {
	CDDB_ACCESS_READWRITE,
	CDDB_ACCESS_READONLY,
	CDDB_ACCESS_NONE
} CDDBSlaveAccess;

struct _CDDBSlavePrivate {
	char *name;
	char *hostname;
	char *server; /* Server address */
	int port; /* Server port */

	CDDBSlaveAccess access;
	BonoboEventSource *event_source;
};

struct _CDDBRequest {
	CDDBSlave *cddb;

	char *uri;
	char *discid;

	char *name;
	char *version;

	char *buf;
	GString *bufstring;
};

typedef enum _ConnectionMode {
	CONNECTION_MODE_NEED_HELLO,
	CONNECTION_MODE_NEED_HELLO_RESPONSE,
	CONNECTION_MODE_NEED_QUERY_RESPONSE,
	CONNECTION_MODE_NEED_READ_RESPONSE,
	CONNECTION_MODE_NEED_GOODBYE
} ConnectionMode;

typedef struct _ConnectionData {
	CDDBSlave *cddb;

	GTcpSocket *socket;
	GIOChannel *iochannel;
	guint tag;
	
	ConnectionMode mode;
	
	char *discid;
	int ntrks;
	char *offsets;
	int nsecs;
	char *name;
	char *version;

	/* A list of discs that match */
	GList *matches;
} ConnectionData;

static GHashTable *pending_requests = NULL;
static GHashTable *cddb_cache = NULL;

static void do_hello (ConnectionData *cd);

#define CDDB_SLAVE_CDDB_FINISHED "GNOME_Media_CDDBSlave2:CDDB-Finished"

/* Notify the listeners on the CDDBSlave that we've finished
   and that they can find the data for @discid in the .cddbslave directory. */
static void
cddb_slave_notify_listeners (CDDBSlave *cddb,
			     const char *discid,
			     GNOME_Media_CDDBSlave2_Result result)
{
	CORBA_any any;
	GNOME_Media_CDDBSlave2_QueryResult qr;

	g_return_if_fail (cddb != NULL);
	g_return_if_fail (IS_CDDB_SLAVE (cddb));

	qr.discid = CORBA_string_dup (discid ? discid : "");
	qr.result = result;

	any._type = (CORBA_TypeCode) TC_GNOME_Media_CDDBSlave2_QueryResult;
	any._value = &qr;

	bonobo_event_source_notify_listeners (cddb->priv->event_source,
					      CDDB_SLAVE_CDDB_FINISHED,
					      &any, NULL);
}

static gboolean
do_goodbye_response (ConnectionData *cd,
		     const char *response)
{
	CDDBSlave *cddb = cd->cddb;
	int code;

	code = atoi (response);
	switch (code) {
	case 230:
		g_print ("Disconnected\n");
		g_print ("%s\n", response);
		break;

	default:
		g_print ("Unknown response\n");
		g_print ("%s\n", response);
		break;
	}

	/* Disconnect */
	if (cd->tag) {
		g_source_remove (cd->tag);
	}
	
  	gnet_tcp_socket_unref (cd->socket);
  	g_io_channel_unref (cd->iochannel);
	
	/* Notify listeners */
	cddb_slave_notify_listeners (cddb, cd->discid, GNOME_Media_CDDBSlave2_OK);

	/* Destroy data */
	g_object_unref (cd->cddb);
	g_free (cd->discid);
	g_free (cd->offsets);
	g_free (cd->name);
	g_free (cd->version);

	if (cd->matches != NULL) {
		GList *p;

		for (p = cd->matches; p; p = p->next) {
			g_strfreev ((char **) p->data);
		}

		g_list_free (cd->matches);
	}
	g_free (cd);
	
	return FALSE;
}

static void
remove_entry_from_cache (const char *discid)
{
	g_hash_table_remove (cddb_cache, discid);
}

static void
do_goodbye (ConnectionData *cd)
{
	char *quit;
	gsize bytes_writen;
	GIOError status;
	
	/* Send quit command */
	status = gnet_io_channel_writen (cd->iochannel, "quit\n",
					 5, &bytes_writen);
	cd->mode = CONNECTION_MODE_NEED_GOODBYE;
}

static gboolean
do_read_response (ConnectionData *cd,
		  const char *response)
{
	CDDBSlave *cddb = cd->cddb;
	int code;
	gboolean more = FALSE;
	gboolean disconnect = FALSE;
	static gboolean waiting_for_terminator = FALSE;
	static FILE *handle = NULL;
	
	if (waiting_for_terminator == TRUE) {
		code = 210;
	} else {
		code = atoi (response);
	}

	switch (code) {
	case 210:
		if (waiting_for_terminator == FALSE) {
			/* Open the file */
			char *filename, *dirname;

			dirname = gnome_util_prepend_user_home (".cddbslave");
			filename = g_concat_dir_and_file (dirname, cd->discid);
			g_free (dirname);

			g_print ("Opening %s\n", filename);
			handle = fopen (filename, "w");
			g_free (filename);
			
			if (handle == NULL) {
				g_print ("Erk!\n");
				return FALSE;
			}

			waiting_for_terminator = TRUE;
		}

		g_assert (handle != NULL);
		
		/* Write the line */
		fputs (response, handle);

		if (response[0] == '.') {
			CDDBEntry *entry;
			char *filename, *dirname;
			
			/* Found terminator */
			fclose (handle);

			dirname = gnome_util_prepend_user_home (".cddbslave");
			filename = g_concat_dir_and_file (dirname, cd->discid);
			g_free (dirname);
			
			entry = g_hash_table_lookup (cddb_cache, cd->discid);
			cddb_entry_parse_file (entry, filename);
			g_free (filename);
			
			/* Reset the static variables for next time */
			handle = NULL;
			more = FALSE;
			disconnect = TRUE;
			waiting_for_terminator = FALSE;
		} else {
			more = TRUE;
		}

		break;

	case 401:
		g_print ("Specified CDDB entry not found\n");
		g_print ("%s\n", response);
		more = FALSE;
		disconnect = TRUE;
		remove_entry_from_cache (cd->discid);
		break;

	case 402:
		g_print ("Server error\n");
		g_print ("%s\n", response);
		more = FALSE;
		disconnect = TRUE;
		remove_entry_from_cache (cd->discid);
		break;

	case 403:
		g_print ("Database entry is corrupt\n");
		g_print ("%s\n", response);
		more = FALSE;
		disconnect = TRUE;
		remove_entry_from_cache (cd->discid);
		break;

	case 409:
		g_print ("No handshake\n");
		g_print ("%s\n", response);

		/* Handshake */
		do_hello (cd);
		more = FALSE;
		break;

	}

	if (disconnect == TRUE) {
		do_goodbye (cd);
	}
		
	return more;
}

static void
do_read (ConnectionData *cd,
	 const char *cat,
	 const char *discid,
	 const char *dtitle)
{
	char *query;
	gsize bytes_writen;
	GIOError status;
	
	/* Send read command */
	query = g_strdup_printf ("cddb read %s %s\n", cat, discid);
	status = gnet_io_channel_writen (cd->iochannel, query,
					 strlen (query), &bytes_writen);
	g_free (query);
	cd->mode = CONNECTION_MODE_NEED_READ_RESPONSE;
}

static GtkTreeModel *
create_model_from_list (GList *list)
{
	GtkListStore *store;
	GtkTreeIter iter;
	GList *l;

	store = gtk_list_store_new (3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);

	for (l = list; l; l = l->next) {
		char **vector;

		vector = (char **) l->data;

		gtk_list_store_append (store, &iter);
		gtk_list_store_set (store, &iter, 0, vector[0], 1, vector[1], 2, vector[2], -1);
	}

	return GTK_TREE_MODEL (store);
}
		
static char **
display_results (ConnectionData *cd)
{
	GtkWidget *window, *list, *sw, *label;
	GtkCellRenderer *cell;
	GtkTreeViewColumn *col;
	GtkTreeModel *model, *selmodel;
	GtkTreeSelection *selection;
	GtkTreeIter iter;
	char **vector;
	int i;
	
	window = gtk_dialog_new_with_buttons (_("Multiple matches..."),
					      NULL, 0,
					      GTK_STOCK_CANCEL, 0, GTK_STOCK_OK, 1, NULL);

	label = gtk_label_new (_("There were multiple matches found in the database.\n"
				 "Below is a list of possible matches, please choose the "
				 "best match"));
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), label, TRUE, TRUE, 0);
	gtk_widget_show (label);
	
	model = create_model_from_list (cd->matches);
	list = gtk_tree_view_new_with_model (model);
	g_object_unref (model);

	cell = gtk_cell_renderer_text_new ();
	col = gtk_tree_view_column_new_with_attributes (_("Category"), cell,
							"text", 0, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (list), col);
	
	col = gtk_tree_view_column_new_with_attributes (_("Disc ID"), cell,
							"text", 1, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (list), col);
	
	col = gtk_tree_view_column_new_with_attributes (_("Artist and Title"), cell,
							"text", 2, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (list), col);
	
	sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw),
					GTK_POLICY_AUTOMATIC,
					GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER (sw), list);
	gtk_widget_show_all (sw);

	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (window)->vbox), sw, TRUE, TRUE, 0);
	switch (gtk_dialog_run (GTK_DIALOG (window))) {
	case 0:
		gtk_widget_destroy (window);
		return NULL;

	case 1:
		selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (list));
		if (gtk_tree_selection_get_selected (selection, NULL, &iter) == TRUE) {
			char **vector;

			vector = g_new (char *, 3);
			gtk_tree_model_get (model, &iter, 0, &vector[0], 1, &vector[1], 2, &vector[2], -1);

			gtk_widget_destroy (window);
			return vector;
		} else {
			gtk_widget_destroy (window);
			return NULL;
		}
		
		gtk_widget_destroy (window);
		return NULL;

	default:
		break;
	}

	gtk_widget_destroy (window);
	return NULL;
}

static gboolean
do_query_response (ConnectionData *cd,
		   const char *response)
{
	CDDBSlave *cddb = cd->cddb;
	int code;
	gboolean more = FALSE;
	char *cat = NULL, *discid = NULL, *dtitle = NULL;
	char **vector;
	static gboolean waiting_for_terminator = FALSE;
	gboolean disconnect = FALSE;
	
	if (waiting_for_terminator == TRUE) {
		code = 211;
	} else {
		code = atoi (response);
	}
	
	switch (code) {
	case 200:
		g_print ("Exact match found.\n");
		g_print ("%s\n", response);

		vector = g_strsplit (response, " ", 4);
		if (vector == NULL) {
			g_print ("Erk!\n");
			return FALSE;
		}

		cat = g_strdup (vector[1]);
		discid = g_strdup (vector[2]);
		dtitle = g_strdup (vector[3]);

		g_strfreev (vector);

		more = FALSE;
		break;

	case 211:

		if (response[0] == '.') {
			/* Terminator */
			char **result;
			GList *l;
			
			waiting_for_terminator = FALSE;
			if (cd->matches && cd->matches->next == NULL) {
				/* There is only one match, even though
				   cddb returned 211 */

				result = cd->matches->data;
				cat = g_strdup (result[0]);
				discid = g_strdup (result[1]);
				dtitle = g_strdup (result[2]);
			} else {
				result = display_results (cd);

				if (result != NULL) {
					cat = result[0];
					discid = result[1];
					dtitle = result[2];
					g_free (result);
					
				} else {
				/* Need to disconnect here...
				   none of our matches matched */
					remove_entry_from_cache (cd->discid);
					do_goodbye (cd);
				}
			}

			/* Free the vector list */
			for (l = cd->matches; l; l = l->next) {
				g_strfreev ((char **) l->data);
			}
			g_list_free (cd->matches);
			cd->matches = NULL;
			
			more = FALSE;
			break;
		}

		if (waiting_for_terminator == TRUE) {
			vector = g_strsplit (response, " ", 3);
			if (vector == NULL) {
				g_print ("Erk!\n");
				return FALSE;
			}
			
			/* Add the vector to the list of matches */
			cd->matches = g_list_append (cd->matches, vector);
		}

		waiting_for_terminator = TRUE;
		more = TRUE;

		break;

	case 202:
		g_print ("No match found\n");
		g_print ("%s\n", response);
		more = FALSE;
		disconnect = TRUE;
		break;

	case 403:
		g_print ("Database entry is corrupt\n");
		g_print ("%s\n", response);
		more = FALSE;
		disconnect = TRUE;
		break;

	case 409:
		g_print ("No handshake\n");
		g_print ("%s\n", response);

		do_hello (cd);
		more = FALSE;
		break;

	default:
		g_print ("Unknown response\n");
		g_print ("%s\n", response);
		disconnect = TRUE;
		more = FALSE;
		break;
	}

	if (cat != NULL &&
	    discid != NULL &&
	    dtitle != NULL &&
	    cddb->priv->access != CDDB_ACCESS_NONE) {
		do_read (cd, cat, discid, dtitle);
	}

	if (disconnect == TRUE) {
		remove_entry_from_cache (cd->discid);
		do_goodbye (cd);
	}
	
	g_free (cat);
	g_free (discid);
	g_free (dtitle);
	return more;
}

static void
do_query (ConnectionData *cd)
{
	char *query;
	gsize bytes_writen;
	GIOError status;
	
	/* Send query command */
	query = g_strdup_printf ("cddb query %s %d %s %d\n",
				 cd->discid, cd->ntrks, 
				 cd->offsets, cd->nsecs);
	status = gnet_io_channel_writen (cd->iochannel, query,
					 strlen (query), &bytes_writen);
	g_print ("status: %d bytes_writen %d\n", status, bytes_writen);
	g_free (query);
	cd->mode = CONNECTION_MODE_NEED_QUERY_RESPONSE;
}

static gboolean
do_hello_response (ConnectionData *cd,
		   const char *response)
{
	CDDBSlave *cddb = cd->cddb;
	int code;

	code = atoi (response);
	switch (code) {
	case 200:
		g_print ("Hello ok - Welcome\n");
		g_print ("%s\n", response);
		break;

	case 431:
		g_print ("Hello unsuccessful\n");
		g_print ("%s\n", response);

		/* Disconnect here */
		remove_entry_from_cache (cd->discid);
		do_goodbye (cd);
		break;

	case 402:
		g_print ("Already shook hands\n");
		g_print ("%s\n", response);
		break;

	default:
		g_print ("Unknown response\n");
		g_print ("%s\n", response);
		break;
	}

	if (cddb->priv->access != CDDB_ACCESS_NONE) {
		do_query (cd);
	}

	return FALSE;
}

static void
do_hello (ConnectionData *cd)
{
	char *hello;
	gsize bytes_writen;
	GIOError status;
	
	/* Send the Hello command
	   CDDB howto says these shouldn't be hardcoded,
	   but that seems to be a privacy issue */
	hello = g_strdup_printf ("cddb hello %s %s %s %s\n",
				 cd->cddb->priv->name,
				 cd->cddb->priv->hostname,
				 cd->name,
				 cd->version);
	
	/* Need to check the return of this one */
	status = gnet_io_channel_writen (cd->iochannel, hello,
					 strlen (hello), &bytes_writen);
	g_print ("Status: %d bytes_writen: %d\n", status, bytes_writen);
	g_free (hello);

	cd->mode = CONNECTION_MODE_NEED_HELLO_RESPONSE;
}

static gboolean
do_open_response (ConnectionData *cd,
		  const char *response)
{
	CDDBSlave *cddb = cd->cddb;
	int code;

	/* did we get the hello? */
	code = atoi (response);
	switch (code) {
	case 200:
		g_print ("Hello ok - Read/Write access allowed\n");
		g_print ("%s\n", response);
		cddb->priv->access = CDDB_ACCESS_READWRITE;
		break;
		
	case 201:
		g_print ("Hello ok - Read only access\n");
		g_print ("%s\n", response);
		cddb->priv->access = CDDB_ACCESS_READONLY;
		break;
		
	case 432:
		g_print ("No more connections allowed\n");
		g_print ("%s\n", response);
		cddb->priv->access = CDDB_ACCESS_NONE;
		break;

	case 433:
		g_print ("No connections allowed: X users allowed, Y currently active\n");
		g_print ("%s\n", response);
		cddb->priv->access = CDDB_ACCESS_NONE;
		break;

	case 434:
		g_print ("No connections allowed: system load too high\n");
		g_print ("%s\n", response);
		cddb->priv->access = CDDB_ACCESS_NONE;
		break;

	default:
		g_print ("Unknown response code: %d\n");
		g_print ("%s\n", response);
		cddb->priv->access = CDDB_ACCESS_NONE;
		break;
	}

	if (cddb->priv->access != CDDB_ACCESS_NONE) {
		do_hello (cd);
	} else {
		/* Do something to indicate that we can't contact server */
		/* Close connection to tell listeners we're not doing anything */
		remove_entry_from_cache (cd->discid);
		do_goodbye (cd);
	}

	return FALSE;
}
	
static gboolean
read_from_server (GIOChannel *iochannel,
		  GIOCondition condition,
		  gpointer data)
{
	ConnectionData *cd = data;

	if (condition & (G_IO_ERR | G_IO_HUP | G_IO_NVAL)) {
		g_warning ("Socket error");

		if (condition & G_IO_ERR) {
			g_print ("G_IO_ERR\n");
		}

		if (condition & G_IO_HUP) {
			g_print ("G_IO_HUP\n");
		}

		if (condition & G_IO_NVAL) {
			g_print ("G_IO_NVAL\n");
		}
		
		goto error;
	}

	if (condition & G_IO_IN) {
		GIOError error;
		char *buffer;
		gsize bytes_read;

		/* Read the data into our buffer */
		error = g_io_channel_read_line (iochannel, &buffer,
						&bytes_read, NULL, NULL);
		while (error == G_IO_STATUS_NORMAL) {
			gboolean more = FALSE;
			
			switch (cd->mode) {
			case CONNECTION_MODE_NEED_HELLO:
				more = do_open_response (cd, buffer);
				break;

			case CONNECTION_MODE_NEED_HELLO_RESPONSE:
				more = do_hello_response (cd, buffer);
				break;

			case CONNECTION_MODE_NEED_QUERY_RESPONSE:
				more = do_query_response (cd, buffer);
				break;

			case CONNECTION_MODE_NEED_READ_RESPONSE:
				more = do_read_response (cd, buffer);
				break;

			case CONNECTION_MODE_NEED_GOODBYE:
				more = do_goodbye_response (cd, buffer);
				break;
				
			default:
				g_print ("Dunno what to do with %s\n", buffer);
				more = FALSE;
				break;
			}

			g_free (buffer);

			if (more == TRUE) {
				error = g_io_channel_read_line (iochannel, &buffer,
								&bytes_read,
								NULL, NULL);
			} else {
				break;
			}
		}
	}

	return TRUE;

 error:
	remove_entry_from_cache (cd->discid);
	return FALSE;
}

static void
open_cb (GTcpSocket *sock,
	 GInetAddr *addr,
	 GTcpSocketConnectAsyncStatus status,
	 gpointer data)
{
	ConnectionData *cd = data;
	GIOChannel *sin;

	cd->socket = sock;
	if (status != GTCP_SOCKET_CONNECT_ASYNC_STATUS_OK) {
		g_warning ("Error opening %s:%s",
			   cd->cddb->priv->server,
			   cd->cddb->priv->port);

		/* notify listeners */
		remove_entry_from_cache (cd->discid);
		return;
	}

	sin = gnet_tcp_socket_get_iochannel (sock);
	cd->iochannel = sin;
	
	cd->tag = g_io_add_watch (sin, G_IO_IN | G_IO_ERR | G_IO_HUP | G_IO_NVAL,
				  read_from_server, data);
}

static void
cddb_send_cmd (ConnectionData *data)
{
	GTcpSocketConnectAsyncID *sock;

	sock = gnet_tcp_socket_connect_async (data->cddb->priv->server,
					      data->cddb->priv->port,
					      open_cb, data);
	if (sock == NULL) {
		g_warning ("Could not connect to %s:%s",
			   data->cddb->priv->server,
			   data->cddb->priv->port);
		/* Notify listeners */
		return;
	}
}
	
static gboolean
cddb_check_cache (const char *discid)
{
	if (g_hash_table_lookup (cddb_cache, discid) == NULL) {
		return FALSE;
	} else {
		return TRUE;
	}
}


/* CORBA implementation */
static inline CDDBSlave *
cddb_slave_from_servant (PortableServer_Servant servant)
{
	return CDDB_SLAVE (bonobo_object_from_servant (servant));
}

static void
impl_GNOME_Media_CDDBSlave2_query (PortableServer_Servant servant,
				   const CORBA_char *discid,
				   const CORBA_short ntrks,
				   const CORBA_char *offsets,
				   const CORBA_long nsecs,
				   const CORBA_char *name,
				   const CORBA_char *version,
				   CORBA_Environment *ev)
{
	CDDBSlave *cddb;
	CDDBEntry *entry;
	ConnectionData *cd;
	char *request, *safe_offsets;
	char *username, *hostname, *fullname, *uri;

	cddb = cddb_slave_from_servant (servant);

	if (cddb_check_cache (discid) == TRUE) {
		cddb_slave_notify_listeners (cddb, discid, GNOME_Media_CDDBSlave2_OK);
		return;
	}

	/* Make an entry */
	entry = cddb_entry_new (discid, ntrks, offsets, nsecs);
	g_hash_table_insert (cddb_cache, g_strdup (discid), entry);
	
	cd = g_new (ConnectionData, 1);
	cd->cddb = cddb;
	g_object_ref (cddb);

	cd->tag = 0;
	
	cd->name = g_strdup_printf ("%s(CDDBSlave2)", name);
	cd->version = g_strdup (version);
	cd->discid = g_strdup (discid);
	cd->ntrks = ntrks;
	cd->offsets = g_strdup (offsets);
	cd->nsecs = nsecs;
	cd->mode = CONNECTION_MODE_NEED_HELLO;

	cd->matches = NULL;
	cddb_send_cmd (cd);
}

static CORBA_char *
impl_GNOME_Media_CDDBSlave2_getArtist (PortableServer_Servant servant,
				       const CORBA_char *discid,
				       CORBA_Environment *ev)
{
	CDDBEntry *entry;
	char *split, *artist;
	GString *dtitle;
	CORBA_char *ret;
	
	entry = g_hash_table_lookup (cddb_cache, discid);
	if (entry == NULL) {
		/* Should set an exception here */
		return NULL;
	}

	dtitle = g_hash_table_lookup (entry->fields, "DTITLE");
	if (dtitle == NULL) {
		return NULL;
	}

	split = strstr (dtitle->str, " / ");
	if (split == NULL) {
		ret = CORBA_string_dup (dtitle->str);
	} else {
		artist = g_strndup (dtitle->str, split - dtitle->str);
		ret = CORBA_string_dup (artist);
		g_free (artist);
	}
	
	return ret;
}

static CORBA_char *
impl_GNOME_Media_CDDBSlave2_getDiscTitle (PortableServer_Servant servant,
					  const CORBA_char *discid,
					  CORBA_Environment *ev)
{
	CDDBEntry *entry;
	char *split;
	GString *dtitle;

	entry = g_hash_table_lookup (cddb_cache, discid);
	if (entry == NULL) {
		return NULL;
	}

	dtitle = g_hash_table_lookup (entry->fields, "DTITLE");
	if (dtitle == NULL) {
		return NULL;
	}

	split = strstr (dtitle->str, " / ");
	if (split == NULL) {
		return CORBA_string_dup ("");
	} else {
		return CORBA_string_dup (split + 3);
	}
}

static CORBA_short
impl_GNOME_Media_CDDBSlave2_getNTrks (PortableServer_Servant servant,
				      const CORBA_char *discid,
				      CORBA_Environment *ev)
{
	CDDBEntry *entry;

	entry = g_hash_table_lookup (cddb_cache, discid);
	if (entry == NULL) {
		return -1;
	}

	return entry->ntrks;
}

static CORBA_char *
impl_GNOME_Media_CDDBSlave2_getTrackTitle (PortableServer_Servant servant,
					   const CORBA_char *discid,
					   CORBA_short track,
					   CORBA_Environment *ev)
{
	CDDBEntry *entry;
	char *name;
	GString *ttitle;
	
	entry = g_hash_table_lookup (cddb_cache, discid);
	if (entry == NULL) {
		return NULL;
	}

	name = g_strdup_printf ("TTITLE%d", track);
	ttitle = g_hash_table_lookup (entry->fields, name);
	g_free (name);
	if (ttitle == NULL) {
		return NULL;
	}

	return CORBA_string_dup (ttitle->str);
}

static void
impl_GNOME_Media_CDDBSlave2_getAllTracks (PortableServer_Servant servant,
					  const CORBA_char *discid,
					  GNOME_Media_CDDBSlave2_StringList **names_list,
					  CORBA_Environment *ev)
{
	CDDBEntry *entry;
	int ntrk;
	
	entry = g_hash_table_lookup (cddb_cache, discid);
	if (entry == NULL) {
		return;
	}

	*names_list = GNOME_Media_CDDBSlave2_StringList__alloc ();
	(*names_list)->_length = 0;
	(*names_list)->_maximum = entry->ntrks + 1;
	(*names_list)->_buffer = CORBA_sequence_CORBA_string_allocbuf (entry->ntrks);

	for (ntrk = 0; ntrk < entry->ntrks; ntrk++) {
		char *name;
		GString *ttitle;

		name = g_strdup_printf ("TTITLE%d", ntrk);
		ttitle = g_hash_table_lookup (entry->fields, name);
		if (ttitle != NULL) {
			(*names_list)->_buffer[ntrk] = CORBA_string_dup (ttitle->str ? ttitle->str : "");
		} else {
			(*names_list)->_buffer[ntrk] = CORBA_string_dup ("");
		}
		(*names_list)->_length++;

		g_free (name);
	}

	return;
}
	

static void
finalize (GObject *object)
{
	CDDBSlave *cddb;
	CDDBSlavePrivate *priv;

	cddb = CDDB_SLAVE (object);
	priv = cddb->priv;

	if (priv == NULL)
		return;

	g_free (priv->server);
	g_free (priv->name);
	g_free (priv->hostname);
	
	g_free (priv);
	cddb->priv = NULL;

	G_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
cddb_slave_class_init (CDDBSlaveClass *klass)
{
	GObjectClass *object_class;
	POA_GNOME_Media_CDDBSlave2__epv *epv = &klass->epv;

	object_class = G_OBJECT_CLASS (klass);
	object_class->finalize = finalize;

	parent_class = g_type_class_peek_parent (klass);
	epv->query = impl_GNOME_Media_CDDBSlave2_query;
	epv->getArtist = impl_GNOME_Media_CDDBSlave2_getArtist;
	epv->getDiscTitle = impl_GNOME_Media_CDDBSlave2_getDiscTitle;
	epv->getNTrks = impl_GNOME_Media_CDDBSlave2_getNTrks;
	epv->getTrackTitle = impl_GNOME_Media_CDDBSlave2_getTrackTitle;
	epv->getAllTracks = impl_GNOME_Media_CDDBSlave2_getAllTracks;
}

static void
cddb_slave_init (CDDBSlave *cddb)
{
	CDDBSlavePrivate *priv;

	priv = g_new0 (CDDBSlavePrivate, 1);
	cddb->priv = priv;

	if (cddb_cache == NULL) {
		cddb_cache = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
	}
}

BONOBO_TYPE_FUNC_FULL (CDDBSlave, GNOME_Media_CDDBSlave2, PARENT_TYPE, cddb_slave);
			 

#define CDDB_SERVER "freedb.freedb.org"
#define CDDB_PORT 888

/**
 * cddb_slave_new_full:
 * @server: Address of server to connect to.
 * @port: Port on @server to connect to.
 * @event_source: #BonoboEventSource on which to emit events.
 *
 * Creates a new #CDDBSlave which will connect to @server:@port. Any events 
 * that are created will be emitted on @event_source.
 * If @server is NULL, the default server (freedb.freedb.org) is used.
 * If @port is 0, the default port (888) is used.
 *
 * Returns: A newly initialised #CDDBSlave, or #NULL on fail.
 */
CDDBSlave *
cddb_slave_new_full (const char *server,
		     int port,
		     const char *name,
		     const char *hostname,
		     BonoboEventSource *event_source)
{
	CDDBSlave *cddb;
	CDDBSlavePrivate *priv;

	g_return_val_if_fail (BONOBO_IS_EVENT_SOURCE (event_source), NULL);

	cddb = g_object_new (cddb_slave_get_type (), NULL);
	priv = cddb->priv;

	if (server == NULL) {
		priv->server = g_strdup (CDDB_SERVER);
	} else {
		priv->server = g_strdup (server);
	}

	if (port == 0) {
		priv->port = CDDB_PORT;
	} else {
		priv->port = port;
	}

	priv->name = g_strdup (name);
	priv->hostname = g_strdup (hostname);
	
	priv->event_source = event_source;
	bonobo_object_add_interface (BONOBO_OBJECT (cddb),
				     BONOBO_OBJECT (priv->event_source));

	return cddb;
}
/**
 * cddb_slave_new:
 * @server: Address of server to connect to.
 * @port: Port on @server to connect to.
 *
 * Creates a new #CDDBSlave which will connect to @server:@port. Any events 
 * that are created will be emitted on the default event source.
 * If @server is NULL, the default server (freedb.freedb.org) is used.
 * If @port is 0, the default port (888) is used.
 *
 * Returns: A newly initialised #CDDBSlave.
 */
CDDBSlave *
cddb_slave_new (const char *server,
		int port,
		const char *name,
		const char *hostname)
{
	BonoboEventSource *event_source;

	event_source = bonobo_event_source_new ();

	return cddb_slave_new_full (server, port, name, hostname, event_source);
}

/**
 * cddb_slave_get_event_source:
 * @cddb: The #CDDBSlave to get the event source from.
 *
 * Gets the event source that @cddb will emit events on.
 *
 * Returns: A #BonoboEventSource.
 */
BonoboEventSource *
cddb_slave_get_event_source (CDDBSlave *cddb)
{
	g_return_val_if_fail (cddb != NULL, NULL);
	g_return_val_if_fail (IS_CDDB_SLAVE (cddb), NULL);

	return cddb->priv->event_source;
}

/**
 * cddb_slave_set_server:
 * @cddb: #CDDBSlave
 * @server: New server to connect to.
 *
 * Sets the server on @cddb to @server. If @server is #NULL sets it to the
 * default (freedb.freedb.org)
 */
void
cddb_slave_set_server (CDDBSlave *cddb,
		       const char *server)
{
	g_return_if_fail (IS_CDDB_SLAVE (cddb));
	
	g_free (cddb->priv->server);
	if (server == NULL) {
		cddb->priv->server = g_strdup (CDDB_SERVER);
	} else {
		cddb->priv->server = g_strdup (server);
	}
}

/**
 * cddb_slave_set_port:
 * @cddb: #CDDBSlave
 * @port: Port to connect to.
 *
 * Sets the port for @cddb to connect to.
 */
void
cddb_slave_set_port (CDDBSlave *cddb,
		     int port)
{
	g_return_if_fail (IS_CDDB_SLAVE (cddb));

	if (port == 0) {
		cddb->priv->port = CDDB_PORT;
	} else {
		cddb->priv->port = port;
	}
}

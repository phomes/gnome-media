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

#include <gtk/gtk.h>

#include <bonobo/bonobo-object.h>
#include <bonobo/bonobo-event-source.h>
#include <bonobo/bonobo-main.h>
#include <bonobo/bonobo-arg.h>
#include <bonobo/bonobo-exception.h>
#include <libgnome/gnome-util.h>

#include <libgnomevfs/gnome-vfs-async-ops.h>
#include <libgnomevfs/gnome-vfs-utils.h>
#include <libgnomevfs/gnome-vfs-result.h>

#include <orbit/orbit.h>

#include "cddb-slave.h"
#include "GNOME_Media_CDDBSlave2.h"


#define PARENT_TYPE BONOBO_OBJECT_TYPE
static BonoboObjectClass *parent_class = NULL;

struct _CDDBSlavePrivate {
	char *server; /* Server address */
	int port; /* Server port */

	BonoboEventSource *event_source;
};

struct _CDDBRequest {
	CDDBSlave *cddb;

	GnomeVFSAsyncHandle *handle;
	GnomeVFSAsyncCloseCallback close_cb;

	char *uri;
	char *discid;

	char *name;
	char *version;

	char *buf;
	GString *bufstring;
};

static GHashTable *pending_requests = NULL;

static void cddb_open_cb (GnomeVFSAsyncHandle *handle,
			  GnomeVFSResult result,
			  gpointer closure);


#define CDDB_SLAVE_CDDB_FINISHED "GNOME_Media_CDDBSlave2:CDDB-Finished"
/* Notify the listeners on the CDDBSlave that we've finished
   and that they can find the data for @discid in the .cddbslave directory. */
static void
cddb_slave_notify_listeners (CDDBSlave *cddb,
			     const char *discid)
{
	CORBA_any any;
	CORBA_short s;

	g_return_if_fail (cddb != NULL);
	g_return_if_fail (IS_CDDB_SLAVE (cddb));

	s = 0;

	any._type = (CORBA_TypeCode) TC_CORBA_short;
	any._value = &s;

	bonobo_event_source_notify_listeners (cddb->priv->event_source,
					      CDDB_SLAVE_CDDB_FINISHED,
					      &any, NULL);
}

static gboolean
cddb_write_data (const char *discid,
		 const char *data)
{
	char *pathname;
	char *filename;
	int fd, remaining;

	pathname = gnome_util_prepend_user_home (".cddbslave");
	filename = g_concat_dir_and_file (pathname, discid);
	g_free (pathname);

	fd = open (filename, O_WRONLY | O_CREAT | O_TRUNC, 0775);
	if (fd < 0) {
		g_warning ("Error opening %s", filename);
		g_free (filename);
		return FALSE;
	}
	g_free (filename);

	remaining = strlen (data);
	while (remaining > 0) {
		int block_size = MIN (remaining, 8192);

		if (write (fd, data, block_size) != block_size) {
			g_warning ("Error writing data");
			close (fd);
			return FALSE;
		}

		data += block_size;
		remaining -= 8192;
	}

	close (fd);
	return TRUE;
}

static void
cddb_close_data_cb (GnomeVFSAsyncHandle *handle,
		    GnomeVFSResult result,
		    gpointer closure)
{
	struct _CDDBRequest *request = (struct _CDDBRequest *) closure;
	char *cddb_result;

	if (request->handle == NULL) {
		/* There was an error in the read callback
		   Destroy the request and remove it from the pending table */
		g_hash_table_remove (pending_requests, request->discid);
		g_string_free (request->bufstring, TRUE);
		g_free (request->buf);
		g_free (request->discid);
		g_free (request->uri);
		g_free (request->name);
		g_free (request->version);

		cddb_slave_notify_listeners (request->cddb, NULL);
		bonobo_object_unref (BONOBO_OBJECT (request->cddb));
		g_free (request);
		
		return;
	}

	cddb_result = request->bufstring->str;
	g_string_free (request->bufstring, FALSE);
	g_free (request->buf);

	/* Finalise everything. 
	   Write data to disk, 
	   Notify the listeners,
	   Remove the pending request
	   Free data */
	if (cddb_write_data (request->discid, cddb_result) == TRUE) {
		/* Only notify listeners if we were able to write it out */
		cddb_slave_notify_listeners (request->cddb, request->discid);
	}

	g_hash_table_remove (pending_requests, request->discid);
	g_free (request->uri);
	g_free (request->name);
	g_free (request->discid);
	g_free (request->version);

	bonobo_object_unref (BONOBO_OBJECT (request->cddb));
	g_free (request);

	return;
}

static void
cddb_send_read (struct _CDDBRequest *request,
		const char *category,
		const char *discid)
{
	CDDBSlave *cddb;
	char *req, *safe_req;
	char *name, *safe_name, *safe_version, *username, *hostname;

	cddb = request->cddb;

	req = g_strdup_printf ("cddb+read+%s+%s", category, discid);

	name = g_strdup_printf ("%s(CDDBSlave2)", request->name);

	/* Replace the uri */
	g_free (request->uri);
	request->uri = g_strdup_printf ("http://%s/~cddb/cddb.cgi?cmd=%s&hello=%s+%s+%s+%s&proto=3",
					cddb->priv->server, req, 
					"Destroy2000YearsofCulture",
					"172.23.65.132",
					name, request->version);
	g_free (req);
	g_free (name);

	/* Set the close callback to the one that handles this request */
	request->close_cb = cddb_close_data_cb;

	gnome_vfs_async_open (&request->handle, request->uri,
			      GNOME_VFS_OPEN_READ, cddb_open_cb, request);
}

/* Takes the string that the query returned, 
   splits it up to see what it says, and acts on it.
   Returns FALSE if there was no match, TRUE otherwise */
static gboolean
cddb_parse_result (struct _CDDBRequest *request,
		   const char *cddb_result)
{
	char *cddb_dup;
	char *category, *discid, *title;
	char *start, *end;
	int result;

	cddb_dup = g_strdup (cddb_result);
	start = cddb_dup;
	end = strchr (start, ' ');
	if (end == NULL) {
		/* Badly formed line */
		g_free (cddb_dup);
		return FALSE;
	}
	*end = 0;

	result = atoi (start);
	if (result != 200) {
		/* Not found */
		g_free (cddb_dup);
		return FALSE;
	}
	start = end + 1;
	if (*start == 0) {
		/* Badly formed line */
		g_free (cddb_dup);
		return FALSE;
	}

	end = strchr (start, ' ');
	if (end == NULL) {
		/* Badly formed line */
		g_free (cddb_dup);
		return FALSE;
	}
	*end = 0;

	if (*(end + 1) == 0) {
		/* Badly formed line */
		g_free (cddb_dup);
		return FALSE;
	}
	
	category = g_strdup (start);
	start = end + 1;
	end = strchr (start, ' ');
	if (end == NULL) {
		/* Badly formed line */
		g_free (category);
		g_free (cddb_dup);
		return FALSE;
	}
	*end = 0;

	if (*(end + 1) == 0) {
		/* Badly formed line */
		g_free (category);
		g_free (cddb_dup);
		return FALSE;
	}

	discid = g_strdup (start);
	title = g_strdup (end + 1);
	g_free (cddb_dup);

	/* If we had multiple results, we could display them, but at the
	   moment, we don't really care about the title and stuff */
	g_warning ("Retrieving data for: %s(%s)", title, discid);

	cddb_send_read (request, category, discid);

	g_free (title);
	g_free (category);
	g_free (discid);

	return TRUE;
}
		
static void
cddb_close_cb (GnomeVFSAsyncHandle *handle,
	       GnomeVFSResult result,
	       gpointer closure)
{
	struct _CDDBRequest *request = (struct _CDDBRequest *) closure;
	char *cddb_result;

	if (request->handle == NULL) {
		/* There was an error in the read callback 
		   Destroy the request and remove it from the pending ht */
		g_hash_table_remove (pending_requests, request->discid);
		g_string_free (request->bufstring, TRUE);
		g_free (request->buf);
		g_free (request->discid);
		g_free (request->uri);
		g_free (request->name);
		g_free (request->version);

		cddb_slave_notify_listeners (request->cddb, NULL);
		bonobo_object_unref (BONOBO_OBJECT (request->cddb));
		g_free (request);

		return;
	}

	cddb_result = request->bufstring->str;
	g_string_free (request->bufstring, FALSE);
	g_free (request->buf);

	if (cddb_parse_result (request, cddb_result) == FALSE) {
		/* Disc not found */
		g_hash_table_remove (pending_requests, request->discid);
		g_free (request->discid);
		g_free (request->uri);
		g_free (request->name);
		g_free (request->version);

		cddb_slave_notify_listeners (request->cddb, NULL);
		bonobo_object_unref (BONOBO_OBJECT (request->cddb));
		g_free (request);
	}

	g_free (cddb_result);
}

/* Displays error to terminal.
   If there is a GUI available, should display error in a dialog
   FIXME  :) */
static void
cddb_slave_error (const char *error,
		  struct _CDDBRequest *request)
{
	g_print ("CDDBSlave %s: %s", request->uri, error);
}

static void
cddb_read_cb (GnomeVFSAsyncHandle *handle,
	      GnomeVFSResult result,
	      gpointer buffer,
	      GnomeVFSFileSize bytes_requested,
	      GnomeVFSFileSize bytes_read,
	      gpointer closure)
{
	struct _CDDBRequest *request = (struct _CDDBRequest *) closure;

	if (result != GNOME_VFS_OK && result != GNOME_VFS_ERROR_EOF) {
		cddb_slave_error (gnome_vfs_result_to_string (result), request);
		request->handle = NULL;
		gnome_vfs_async_close (handle, request->close_cb, request);
	}

	if (bytes_read == 0) {
		/* EOF */
		gnome_vfs_async_close (handle, request->close_cb, request);
	} else {
		/* Add a NULL to the end */
		*((char *) buffer + bytes_read) = 0;
		g_string_append (request->bufstring, (const char *) buffer);
		gnome_vfs_async_read (handle, buffer, 4095, cddb_read_cb,
				      request);
	}
}
		
static void
cddb_open_cb (GnomeVFSAsyncHandle *handle,
	      GnomeVFSResult result,
	      gpointer closure)
{
	struct _CDDBRequest *request = (struct _CDDBRequest *) closure;

	if (result != GNOME_VFS_OK) {
		cddb_slave_error (gnome_vfs_result_to_string (result), request);
		g_hash_table_remove (pending_requests, request->discid);
		g_free (request->discid);
		g_free (request->uri);
		g_free (request->name);
		g_free (request->version);

		cddb_slave_notify_listeners (request->cddb, NULL);
		bonobo_object_unref (BONOBO_OBJECT (request->cddb));
		g_free (request);

		return;
	}

	/* Next level */
	
	request->buf = g_new (char, 4096);
	request->bufstring = g_string_new ("");
	
	gnome_vfs_async_read (handle, request->buf, 4095, cddb_read_cb, request);
}

static void
cddb_send_cmd (CDDBSlave *cddb,
	       const char *uri,
	       const char *discid,
	       const char *name,
	       const char *version)
{
	struct _CDDBRequest *request;

	g_return_if_fail (cddb != NULL);
	g_return_if_fail (IS_CDDB_SLAVE (cddb));
	g_return_if_fail (uri != NULL);

	if (pending_requests == NULL) {
		pending_requests = g_hash_table_new (g_str_hash, g_str_equal);
		request = NULL;
	} else {
		request = g_hash_table_lookup (pending_requests, discid);
	}

	if (request != NULL) {
		/* Request is already happening. We don't want to send 2
		   identical requests */	
		cddb_slave_notify_listeners (request->cddb, NULL);
		return;
	}
	
	request = g_new (struct _CDDBRequest, 1);
	/* Ref the cddb object, unref it when the request is complete */
	request->cddb = cddb;
	bonobo_object_ref (BONOBO_OBJECT (cddb));

	request->uri = g_strdup (uri);
	request->discid = g_strdup (discid);
	request->name = g_strdup (name);
	request->version = g_strdup (version);

	/* Set the close callback to the one to deal with this request */
	request->close_cb = cddb_close_cb;

	g_hash_table_insert (pending_requests, request->discid, request);

	gnome_vfs_async_open (&request->handle, request->uri,
			      GNOME_VFS_OPEN_READ, cddb_open_cb, request);
}

static gboolean
cddb_check_cache (const char *discid)
{
	char *dirname, *discname;

	g_return_val_if_fail (discid != NULL, FALSE);

	dirname = gnome_util_prepend_user_home (".cddbslave");
	if (g_file_test (dirname, G_FILE_TEST_IS_DIR) == FALSE) {
		/* Cache dir doesn't exist */
		mkdir (dirname, S_IRWXU);
		return FALSE;
	}

	discname = g_concat_dir_and_file (dirname, discid);
	g_free (dirname);
	if (g_file_test (discname, G_FILE_TEST_IS_REGULAR) == TRUE) {
		g_free (discname);
		return TRUE;
	} else {
		g_free (discname);
		return FALSE;
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
	char *request, *safe_request, *safe_name, *safe_version;
	char *username, *hostname, *fullname, *uri;

	cddb = cddb_slave_from_servant (servant);

	g_warning ("Request: %s", discid);
	/* Do stuff */
	if (cddb_check_cache (discid) == TRUE) {
		cddb_slave_notify_listeners (cddb, discid);
		return;
	}

	fullname = g_strdup_printf ("%s(CDDBSlave2)", name);

	request = g_strdup_printf ("cddb+query+%s+%d+%s+%d",
				   discid, ntrks, offsets, nsecs);

	uri = g_strdup_printf ("http://%s/~cddb/cddb.cgi?cmd=%s&hello=%s+%s+%s+%s&proto=3",
			       cddb->priv->server, 
			       request, 
			       "Destroy2000YearsofCulture",
			       "172.23.65.132",
			       fullname, version);
	g_free (request);
	g_free (fullname);

	cddb_send_cmd (cddb, uri, discid, name, version);
	g_free (uri);
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
}

static void
cddb_slave_init (CDDBSlave *cddb)
{
	CDDBSlavePrivate *priv;

	priv = g_new0 (CDDBSlavePrivate, 1);
	cddb->priv = priv;
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
		int port)
{
	BonoboEventSource *event_source;

	event_source = bonobo_event_source_new ();

	return cddb_slave_new_full (server, port, event_source);
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

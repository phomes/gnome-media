#include <config.h>
#include <gnome.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <dirent.h>
#include <signal.h>
#include <errno.h>
#include <pwd.h>

#include "socket.h"
#include "cddb.h"

void read_query(char *s, int fd, int len);
int check_response(char *buf);
void disconnect(int fd);
static char *if_strtok(char *p, const char *p2);
char *get_file_name(char *discid);
void getusername_safe(char *buf, int len);
void gethostname_safe(char *buf, int len);
void die(int signal);
int do_request(char *req, int fd);

int check_cache(char *req);
int create_cache(char *req);
int remove_cache(char *req);
char *cache_name(char *req);

char hostname[512], username[512];
char client_ver[64], client_name[64];
char *g_req;
int pid;

gchar *server;
gint port;

const gchar *status_msg[] = {
    N_("Ready for command: %s\n"),
    N_("Connecting [%s]\n"),
    N_("Querying server [%s]\n"),
    N_("Waiting for reply [%s]\n"),
    N_("Reading reply [%s]\n"),
    N_("Disconnecting [%s]\n"),
    N_("Error connecting [%s]\n"),
    N_("Error querying [%s]\n"),
    N_("Error reading [%s]\n"),
    N_("No match found for %s\n"),
    N_("Timeout: %s\n"),
    N_("Done. %s\n"),
};

int main(int argc, char *argv[])
{
    char req[512], client[512];
    int cddev;

    fgets(req, 511, stdin);	/* grab the cddb request */
    fgets(client, 511, stdin);	/* and then the client info */
    
    sscanf(client, "client %s %s %d %d\n", client_name, client_ver, &pid, &cddev);

    gnomelib_init("cddbslave", VERSION);
    server = gnome_config_get_string("/cddbslave/server/address=us.cddb.com");
    port = gnome_config_get_int("/cddbslave/server/port=8880");

    gethostname_safe(hostname, 512);
    getusername_safe(username, 512);
    close(cddev);

    if(check_cache(req))
	exit(0);
    create_cache(req);

    if(fork()==0)
    {
	int fd;
	char *dname;
	DIR *d;
	struct dirent *de;

	/* connect to cddb server */
	g_req = req;
	set_status(STATUS_CONNECTING, server);
	fd = opensocket(server, port);
	if(fd < 0)
	{
	    set_status(ERR_CONNECTING, server);
	    sleep(5);
	    set_status(STATUS_NONE, "");
	    remove_cache(req);
	    return(-1);
	}
	set_status(STATUS_QUERYING, server);

	/* grab directory name */
	dname = get_file_name("");
	if(!dname)
	{
	    if(do_request(req, fd) < 0)
		sleep(5);
	    set_status(STATUS_NONE, "");
	    remove_cache(req);
	    disconnect(fd);
	    exit(-1);
	}

	/* open directory for scanning */
	d = opendir(dname);
	if(!d)
	{
	    if(do_request(req, fd) < 0)
		sleep(5);
	    set_status(STATUS_NONE, "");
	    remove_cache(req);
	    disconnect(fd);
	    exit(-1);
	}

	/* scan for cached queries */
	while((de=readdir(d)))
	{
	    if(strstr(de->d_name, ".query"))
	    {
		FILE *fp;
		char *fname;
		char buf[512];
		
		fname = get_file_name(de->d_name);

		fp = fopen(fname, "r");
		if(!fp)
		    break;
		
		fgets(buf, 512, fp);
		fclose(fp);
		if(do_request(buf, fd) < 0)
		    sleep(5);	/* sleep for a bit so client can get error code */
		remove_cache(buf);
		g_free(fname);
	    }
	}
	/* clean up */
	set_status(STATUS_NONE, "");
	g_free(dname);
	disconnect(fd);
    }
    exit(EXIT_SUCCESS);
}

int do_request(char *req, int fd)
{
    int i;
    char buf[512];

    g_req = req;		/* make the request available to die() */
    
    /* receive login banner */
    fgetsock(buf, 511, fd);
    
    if((i=check_response(buf)) > 400)
	return -i;

    /* send handshake */
    g_snprintf(buf, 511, "cddb hello %s %s %s %s\n", 
	       username, hostname, 
	       client_name, client_ver);
    send(fd, buf, strlen(buf), 0);
    
    /* receive handshake result */
    fgetsock(buf, 511, fd);
    
    if((i=check_response(buf)) != 200)
    {
	set_status(ERR_QUERYING, server);
	return -i;
    }
    /* we're connected and identified. */
    /* now we send our query */
    g_snprintf(buf, 511, "%s\n", req);
    send(fd, buf, strlen(buf), 0);
    set_status(STATUS_READING, server);
    /* receive query result */
    fgetsock(buf, 511, fd);
    
    if((i=check_response(buf)) != 200)
    {
	set_status(ERR_NOMATCH, "disc");
	return -i;
    }
    /* alright, our query response was positive, now send the read request. */
    read_query(buf, fd, 512);
    set_status(STATUS_NONE, "");
    kill(pid, SIGUSR1);
    return 0;
}

int check_response(char *buf)
{
    int code;

    if(sscanf(buf, "%d", &code) != 1)
	return -1;
    
    return code;
}

void disconnect(int fd)
{
    char buf[512];

    /* send quit message */
    g_snprintf(buf, 511, "quit\n");
    send(fd, buf, strlen(buf), 0);
    
    /* receive quit result */
    fgetsock(buf, 511, fd);

    close(fd);
    return;
}

static char *if_strtok(char *p, const char *p2)
{
        p=strtok(p,p2);
        if(p==NULL)
                return("????");
        return p;
}

void read_query(char *s, int fd, int len)
{
    char buf[512];
    char categ[CATEG_MAX];
    char discid[DISCID_MAX];
    char dtitle[DTITLE_MAX];
    char *fname;
    int code;
    FILE *fp;

    s[len-1] = 0;
 
    code = atoi(strtok(s, " " ));         
    strncpy(categ, if_strtok(NULL, " ") , CATEG_MAX);
    categ[CATEG_MAX-1]=0;
    strncpy(discid, if_strtok( NULL, " "), DISCID_MAX);
    discid[DISCID_MAX-1]=0;
    strncpy(dtitle, if_strtok( NULL, "\0"), DTITLE_MAX);
    dtitle[DTITLE_MAX-1]=0;
    
    g_snprintf(buf, 511, "cddb read %s %s\n", categ, discid);
    send(fd, buf, strlen(buf), 0);
    
    fgetsock(buf, 511, fd);
    
    if(check_response(buf) != 210)
	return;
    
    fname = get_file_name(discid);
    if(fname == NULL)
	return;

    fp = fopen(fname, "w");

    if(fp == NULL)
	return;			/* blah, can't open file */

    do
    {
	fgetsock(buf, 511, fd);
	fprintf(fp, "%s", buf);
    } while(strncmp(".", buf, 1));
   
    fclose(fp);
    g_free(fname);
    return;
}

char *get_file_name(char *discid)
{
    char *fname;
    char *homedir=NULL;
    struct stat fs;

    homedir = g_get_home_dir();
    fname = g_malloc(512);

    /* Make our directory if it doesn't exist. */
    g_snprintf(fname, 511, "%s/.cddbslave", homedir);
    g_print("Fname: %s\n", fname);
    if(stat(fname, &fs) < 0)
    {
	if(errno == ENOENT)
	{
	    if(mkdir(fname, S_IRWXU))
	    {
		return NULL;
	    }
	}
	else
	    return NULL;
    }

    g_snprintf(fname, 511, "%s/.cddbslave/%s", homedir, discid);
    return fname;
}

void gethostname_safe(char *buf, int len)
{
    gethostname(buf, 512);
    if(!buf || (strcmp(buf, "")==0))
	strncpy(buf, "generic", len);
}

void getusername_safe(char *buf, int len)
{
    strncpy(buf, g_get_user_name(), len);
}

char *cache_name(char *req)
{
    char discid[64];
    char *fname;
    
    if(sscanf(req, "cddb query %[0-9a-fA-F]", discid) != 1)
	return FALSE;
    
    strncat(discid, ".query", 63);
    
    fname = get_file_name(discid);
    return fname;
}
   
/* Deal with cached requests. */
int check_cache(char *req)
{
    char *fname;
    FILE *fp;

    fname = cache_name(req);
    if(!fname)
	return FALSE;

    fp = fopen(fname, "rw");
    if(!fp)
	return FALSE;		/* query already has been submitted */
    
    fclose(fp);
    g_free(fname);

    return TRUE;
}

/* Create `lock' file. */
int create_cache(char *req)
{
    char *fname;
    FILE *fp;

    fname = cache_name(req);
    if(!fname)
	return FALSE;
    
    fp = fopen(fname, "w");
    if(!fp)
	return FALSE;		/* can't open file */

    fputs(req, fp);

    fclose(fp);
    g_free(fname);

    return TRUE;
}

/* Remove fulfulled queries */
int remove_cache(char *req)
{
    char *fname;
    
    fname = cache_name(req);
    if(!fname)
	return FALSE;
    
    if(remove(fname))
	return FALSE;		/* can't remove file */

    g_free(fname);

    return TRUE;
}

/* FIXME: this assumes that /tmp is your temp dir. */
void set_status(int status, gchar *info)
{
    char tmp[512];
    FILE *fp;

    g_snprintf(tmp, 511, "%s/.cddbstatus", g_get_tmp_dir());
    g_print("tmp: %s\n", tmp);
    if(status == STATUS_NONE)
	truncate(tmp, 0);
    
    fp = fopen(tmp, "a+");
    if(!fp)
    {
	perror("fopen");
	return;
    }
    fprintf(fp, "%03d ", status);
    fprintf(fp, status_msg[status], info);
    fclose(fp);
}

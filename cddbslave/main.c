#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
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

int main(int argc, char *argv[])
{
    char req[512], client[512];
    char client_ver[64], client_name[64];
    int pid;
    
    fgets(req, 511, stdin);	/* grab the cddb request */
    fgets(client, 511, stdin);	/* and then the client info */
    
    sscanf(client, "client %s %s %d\n", client_name, client_ver, &pid);
    
    if(fork()==0)
    {
	int fd, i;
	char buf[512];

	fd = opensocket("cddb.cddb.com", 888);
	if(fd < 0)
	{
	    return(-1);
	}
	
	/* receive login banner */
	fgetsock(buf, 511, fd);
	
	if((i=check_response(buf)) > 400)
	{
	    disconnect(fd);
	    return i;
	}

	/* send handshake */
	g_snprintf(buf, 511, "cddb hello timg foozbar.org %s %s\n", client_name, client_ver);
	send(fd, buf, strlen(buf), 0);

	/* receive handshake result */
	fgetsock(buf, 511, fd);

	if((i=check_response(buf)) != 200)
	{
	    disconnect(fd);
	    return i;
	}

	/* we're connected and identified. */
	/* now we send our query */
	g_snprintf(buf, 511, "%s\n", req);
	send(fd, buf, strlen(buf), 0);

	/* receive query result */
	fgetsock(buf, 511, fd);

	if((i=check_response(buf)) != 200)
	{
	    return -1;
	}

	/* alright, our query response was positive, now send the read request. */
	read_query(buf, fd, 512);

	disconnect(fd);
	kill(pid, SIGUSR1);
    }

    exit(EXIT_SUCCESS);
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
   
    g_free(fname);
    return;
}

char *get_file_name(char *discid)
{
    char *fname;
    char *homedir=NULL;
    struct passwd *pw=NULL;
    struct stat fs;

    homedir = getenv("HOME");

    if(homedir == NULL)
    {
	pw = getpwuid(getuid());

	if(pw != NULL)
	    homedir = pw->pw_dir;
	else
	    homedir = "/";
    }

    fname = g_malloc(512);

    /* Make our directory if it doesn't exist. */
    g_snprintf(fname, 511, "%s/.cddbslave", homedir);
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


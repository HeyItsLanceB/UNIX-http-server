/*
 * Lance Burgo
 * 
 * sws - simple web server
 *
 */


#define _XOPEN_SOURCE 

#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <arpa/inet.h>
#include <dirent.h>
#include <err.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include "cgi.h"
#include "dlist.h"
#include "global.h"
#include "incoming.h"
#include "logging.h"
#include "response.h"

#include <time.h>


/* parseIncoming() will parse all incoming headers. It is important to note that
 * at this point all headers are discarded and only the request line is 
 * reviewed.
 */
void parseIncoming(int sock, struct http_request *req)
{
	struct tm iftime;
	char c;
	char line[BUF_SIZE];
	char *method_tok, *path_tok, *http_ver_tok;
	int i=0, line_no=0;

	/* default status, if there is a problem it will be changed */
	req->status=MSG_OK;

	for (;;) {

		bzero(line,sizeof(line));
		i=0;

		/* get the request, character by character */
		while ( recv(sock,&c,1,0)>0  )
		{
			line[i] = c;

			/* Check for a consecutive CRLF, signifying end of the header */
			if ( (i==1) && (line_no>0) ) {
				if ( (line[i-1]=='\r') && (line[i]=='\n') ) {
						return;
				}
			}

			/* Check for end of line */
			if ( (i>0) && (line[i-1]=='\r') && (line[i]=='\n') ) {
				break;
			}

			i++;

			/* don't let the request exceed the buffer size */
			if (i>BUF_SIZE) {
				req->status = MSG_BAD_REQ;
				break;
			}

		}

		/* This section will parse the request line */
		if (line_no==0) {

			/* remove CRLF */
			line[strlen(line)-1]='\0';
			line[strlen(line)-1]='\0';

			logTime(req);
			logRequest(req,line);

			/* Find out the method */
			method_tok=strtok(line," ");
			if (method_tok==NULL) {
				req->status = MSG_BAD_REQ;
				break;
			} else if (strcmp(method_tok,"GET")==0) {
				req->method=MTHD_GET;
			} else if (strcmp(method_tok,"HEAD")==0) {
				req->method=MTHD_HEAD;
			} else if (strcmp(method_tok,"POST")==0) {
				req->method=MTHD_POST;
			} else {
				req->method=-1;
			} 

			/* Find out the path of the requested file */
			path_tok=strtok(NULL," ");
			strcpy(req->path,path_tok);

			/* Find out if the request is full or simple */
			http_ver_tok=strtok(NULL," ");
			if ( http_ver_tok==NULL ) {
				req->flag_simple=1;
			} else if ( (strcmp(http_ver_tok,"HTTP/1.0")==0)
			            || (strcmp(http_ver_tok,"HTTP/1.1")==0) ) {
				/* default, so do nothing */
			} else {
				req->status = MSG_BAD_REQ;
			}

			/* There should be nothing else on this line */
			if ( strtok(NULL," ") != NULL ) {
				req->status = MSG_BAD_REQ;
			}
		}  /*end of request line parsing*/


		if (line_no>0) {
			if (strncmp( "If-Modified-Since",
			              line,
			              strlen("If-Modified-Since")) ==0 ) {

				memset(&iftime, 0, sizeof(struct tm));

				if ( (strptime( line+strlen("If-Modified-Since: XXX, "),
				          "%d %b %Y %H:%M:%S %z",
				          &iftime))==NULL ) {
					warn("unable to parse time from request header");
				} else {
					req->flag_if_mod=1;
					req->if_time = mktime(&iftime);
				}
			}

		}


		line_no++;
	}
}


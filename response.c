/*
 * Lance Burgo
 * 
 * sws - simple web server
 *
 */

#include <sys/socket.h>
#include <sys/stat.h>

#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "cgi.h"
#include "global.h"
#include "logging.h"
#include "response.h"

/* adds time in standard web format, as opposed to asctime() format. After
 * creating this I learned about the strftime() fuction that is available, which
 * does the exact same thing.
 */
void addTime(char *buf, time_t *time_val)
{
	struct tm *time_st;
	char time_str[50];
	char temp_str[50];

	bzero(time_str,sizeof(time_str));
	time_st=gmtime(time_val);

	switch (time_st->tm_wday) {
		case 0:
			strcpy(time_str,"Sun, ");
			break;
		case 1:
			strcpy(time_str,"Mon, ");
			break;
		case 2:
			strcpy(time_str,"Tue, ");
			break;
		case 3:
			strcpy(time_str,"Wed, ");
			break;
		case 4:
			strcpy(time_str,"Thu, ");
			break;
		case 5:
			strcpy(time_str,"Fri, ");
			break;
		case 6:
			strcpy(time_str,"Sat, ");
			break;
	}

	bzero(temp_str,sizeof(temp_str));
	sprintf(temp_str,"%d ",time_st->tm_mday);
	strcat(time_str,temp_str);

	switch (time_st->tm_mon) {
		case 0:
			strcat(time_str,"Jan ");
			break;
		case 1:
			strcat(time_str,"Feb ");
			break;
		case 2:
			strcat(time_str,"Mar ");
			break;
		case 3:
			strcat(time_str,"Apr ");
			break;
		case 4:
			strcat(time_str,"May ");
			break;
		case 5:
			strcat(time_str,"Jun ");
			break;
		case 6:
			strcat(time_str,"Jul ");
			break;
		case 7:
			strcat(time_str,"Aug ");
			break;
		case 8:
			strcat(time_str,"Sep ");
			break;
		case 9:
			strcat(time_str,"Oct ");
			break;
		case 10:
			strcat(time_str,"Nov ");
			break;
		case 11:
			strcat(time_str,"Dec ");
			break;
	}

	bzero(temp_str,sizeof(temp_str));
	sprintf( temp_str,"%d %d:%d:%d GMT",
	         (time_st->tm_year)+1900,
	         time_st->tm_hour,
	         time_st->tm_min,
	         time_st->tm_sec             );
	strcat(time_str,temp_str);

	strcat(buf,time_str);

	return;
}


/* returns status message for given status code */
char *getStatusStr(int status)
{
	switch(status) {
		case MSG_OK:
			return "OK";
		case MSG_BAD_REQ:
			return "Bad Request";
		case MSG_NOT_FOUND:
			return "Not Found";
		case MSG_NOT_IMPL:
			return "Not Implemented";
		default:
			return "error - invalid code";
	}
}

/* sendResource will the send the resource, whatever it may be, off the client
 * computer
 */
void sendResource(int sock, struct http_request *req)
{
	char send_buf[BUF_SIZE];
	int resource, bytes_sent=0, bytes_read;

	/* if we're good, send the resource */
	if(req->status==MSG_OK) {

		if (req->type==CGI_REQ || req->flag_dir_list) {
			resource=req->pipefd[0];
		} else {
			if ((resource=open(req->path,O_RDONLY))<0) {
				err(EXIT_FAILURE,"cannot open resource");
			}
		}

		while( (bytes_read=read(resource,send_buf,sizeof(send_buf)))>0 )
		{
			if (bytes_read<0) {
				err(EXIT_FAILURE,"error reading from resource");
			}
			if ((bytes_sent+=send(sock,send_buf,strlen(send_buf),0))<0) {
				err(EXIT_FAILURE,"error sending resource");
			}
		}

		if (req->type==CGI_REQ || req->flag_dir_list) {
			close(req->pipefd[0]);
		}

	/* if there's a problem, send the error */
	} else {
		bzero(send_buf,sizeof(send_buf));
		strcpy(send_buf,"<!DOCTYPE HTML>\n");
		strcat(send_buf,"<html>\n");
		strcat(send_buf,"<head>\n");
		strcat(send_buf,"<title>ERROR</title>\n");
		strcat(send_buf,"</head>\n");
		strcat(send_buf,"<body>\n");
		strcat(send_buf,"<h1>Uh-oh! Looks like a problem.</h1>\n");

		if ((bytes_sent+=send(sock,send_buf,strlen(send_buf),0))<0) {
			warn("error sending resource");
		}

		bzero(send_buf,sizeof(send_buf));
		sprintf( send_buf,
		         "<p>Error %d: %s</p>\n",
		         req->status,
		         getStatusStr(req->status) );
		strcat(send_buf,"</body>\n");
		strcat(send_buf,"</html>\n");

		if ((bytes_sent+=send(sock,send_buf,strlen(send_buf),0))<0) {
			warn("error sending resource");
		}
	}

	/* log the size of the request */
	logSize(req,bytes_sent);

	return;
}


/* sendRespHeader() will send the response header. */
void sendRespHeader(int sock, struct http_request *req)
{
	time_t current_time;
	struct stat resource_stat;
	char send_buf[BUF_SIZE];
	int content_size;

	if ( lstat(req->path,&resource_stat)<0 ) {
		req->status=MSG_NOT_FOUND;
	}

	/* find content size */
	if (req->flag_dir_list) {
		content_size=req->content_size;
	} else if (req->status==MSG_OK) {
		content_size=(int)resource_stat.st_size;
	} else {
		content_size=ERR_MSG_LEN+strlen(getStatusStr(req->status));
	}

	/* Send status message */
	bzero(send_buf,sizeof(send_buf));
	sprintf( send_buf,
	         "HTTP/1.0 %d %s\r\n",
	         req->status,
	         getStatusStr(req->status) );
	if (send(sock,send_buf,strlen(send_buf),0)<0) {
		err(EXIT_FAILURE,"problem with send()");
	}

	/* log the status of the current request */
	logStatus(req);

	/* send current date */
	bzero(send_buf,sizeof(send_buf));
	time(&current_time);
	strcat(send_buf, "Date: ");
	addTime(send_buf,&current_time);
	strcat(send_buf,"\r\n");
	if (send(sock,send_buf,strlen(send_buf),0)<0) {
		err(EXIT_FAILURE,"problem with send()");
	}

	/* send server message */
	bzero(send_buf,sizeof(send_buf));
	strcpy(send_buf,"Server: sws v0.1\r\n");
	if (send(sock,send_buf,strlen(send_buf),0)<0) {
		err(EXIT_FAILURE,"problem with send()");
	}

	/* additional headers for non-CGI requests */
	if (req->type!=CGI_REQ) {

		/* send last modidified message */
		bzero(send_buf,sizeof(send_buf));
		strcat(send_buf,"Last-Modified: ");
		if (req->status==MSG_OK) {
			addTime(send_buf,(time_t*)&(resource_stat.st_mtime));
		} else {
			time_t now;
			if ( (time(&now))<0 )
				err(EXIT_FAILURE,"cannot get time");
			addTime(send_buf,&now);
		}
		strcat(send_buf,"\r\n");
		if (send(sock,send_buf,strlen(send_buf),0)<0) {
			err(EXIT_FAILURE,"problem with send()");
		}

		/* send content type message */
		bzero(send_buf,sizeof(send_buf));
		strcpy(send_buf,"Content-Type: text/html\r\n");
		if (send(sock,send_buf,strlen(send_buf),0)<0) {
			err(EXIT_FAILURE,"problem with send()");
		}

		/* send content size message */
		bzero(send_buf,sizeof(send_buf));
		sprintf(send_buf,"Content-Length: %d\r\n",content_size);
		if (send(sock,send_buf,strlen(send_buf),0)<0) {
			err(EXIT_FAILURE,"problem with send()");
		}
		
		/* additional CRLF per spec */
		bzero(send_buf,sizeof(send_buf));
		strcpy(send_buf,"\r\n");
		if (send(sock,send_buf,strlen(send_buf),0)<0) {
			err(EXIT_FAILURE,"problem with send()");
		}
	}

	return;
}

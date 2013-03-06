/*
 * Lance Burgo
 * 
 * sws - simple web server
 *
 */


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



/* checkResource() will check that the resource exists, move the fork to the
 * appropriate location, and in the case of CGI, will parse out the query
 * string from the resource path/name.
 */
int checkResource(struct http_request *req)
{
	char temp_path[PATH_MAX], temp_path2[PATH_MAX], *user_tok;
	struct stat st, temp_st;
	int user_gap, user_len;

	/* CGI requests */
	if ( (flag_allow_cgi) && (strncmp(req->path,CGI_LOC,strlen(CGI_LOC))==0) ) {
		req->type=CGI_REQ;

		/* path must not be only a folder, must request an executable */
		if ( strlen(CGI_LOC)==strlen(req->path) ) {
			req->type=STD_REQ;
			req->status=MSG_NOT_FOUND;
			return -1;
		}

		/* remove cgi-bin from the path */
		strcpy(temp_path,req->path);
		strcpy(req->path,temp_path+(strlen(CGI_LOC)-1));

		/* move to CGI directory */
		if (chdir(cgi_path)<0) {
			err(EXIT_FAILURE,"cannot access cgi-bin directory");
		}

		parseCGI(req);  /* this will parse out path from query string */

	/* requests for a user directory */
	} else if (    (strncmp(req->path,"/~",2)==0)
	            || (strncmp(req->path,"~",1) ==0) ) {

		req->type=USER_REQ;

		/* find out how much space needs to be removed from path */
		if ( (strncmp(req->path,"/~",2)==0) ) {
			user_gap=2;
		} else {
			user_gap=1;
		}

		strcpy(temp_path,req->path+user_gap);
		user_tok = strtok(temp_path,"/");
		user_len=strlen(user_tok);

		/*MOVE to the directory*/
		bzero(temp_path2,sizeof(temp_path2));
		sprintf(temp_path2,"/home/%s/%s",user_tok,USER_LOC);
		if ( (lstat(temp_path2,&st))<0 ) {
			req->status=MSG_NOT_FOUND;
			return -1;
		}
		if (chdir(temp_path2)<0) {
			err(EXIT_FAILURE,"cannot access the user directory");
		}

		bzero(temp_path,sizeof(temp_path));
		strcpy(temp_path,req->path+user_gap);
		/* set the path correctly now that we have moved to teh directory */
		if ( strlen(temp_path)==user_len ) {
			strcpy(req->path,"/");
		} else {
			strcpy(req->path,temp_path+user_len);
		}


	/* standard requests (i.e. non-cgi & non-user directory) */
	} else {
		req->type=STD_REQ;

		if (chdir(serv_path)<0) {
			err(EXIT_FAILURE,"cannot access the user directory");
		}
	}

	/* Remove leading slash, unless it is the only character */
	if ( (strlen(req->path)>1) && (req->path[0]=='/') ) {
		bzero(temp_path,sizeof(temp_path));
		strcpy(temp_path,req->path);
		bzero(req->path,sizeof(req->path));
		strcpy(req->path,temp_path+1);
	}

	/* check that the resource exists */
	if ( (lstat(req->path,&st))<0 ) {
		req->status = MSG_NOT_FOUND;
		return -1;
	} else {
		req->status = MSG_OK;
	}

	if ( st.st_mtime < (req->if_time) ) {
		req->status=MSG_NO_MOD;
		return -1;
	}

	/* handling requests for a directory */
	if (S_ISDIR(st.st_mode)) {

		/* should not get requests for directories in cgi-bin */
		if ( req->type==CGI_REQ ) {
			req->type=STD_REQ;
			req->status=MSG_NOT_FOUND;
			return -1;
		}

		/* add trailing slash to all directories */
		if ( (req->path[strlen(req->path)-1])!='/' ) {
			strcat(req->path,"/");
		}

		/* look for index.html */
		bzero(temp_path,sizeof(temp_path));
		if ( strlen(req->path)==1 && (strcmp(req->path,"/")==0) ) {
			strcpy(temp_path,"index.html");
		} else {
			strcpy(temp_path,req->path);
			strcat(temp_path,"index.html");
		}

		if ( (lstat(temp_path,&temp_st))<0 ) {
			req->flag_dir_list=1;
		} else {
			bzero(req->path,sizeof(req->path));
			strcpy(req->path,temp_path);
		}
	}

	return 1;
}



/* this is the centralized function that handles all incoming requests */
void processRequest(int incoming_sock)
{
	struct http_request req;

	/* set default flags for request */
	req.type=STD_REQ;
	req.status=MSG_OK;
	req.flag_dir_list = 0;
	req.flag_simple   = 0;
	req.flag_if_mod   = 0;

	/*log client IP*/
	logAddress(&req,incoming_sock);

	/* get resource path and method) */
	parseIncoming(incoming_sock, &req);

	/* if it's not already a bad request 400, check the status */
	if (req.status!=MSG_BAD_REQ) {
		checkResource(&req);
	}

	if ( (req.status==MSG_OK) && (req.type==CGI_REQ) ) {
		if (pipe(req.pipefd)<0 ) {
			err(EXIT_FAILURE, "cannot create pipe" );
		}
		execCGI(&req);
	}

	if (req.flag_dir_list) {
		if (pipe(req.pipefd)<0 ) {
			err(EXIT_FAILURE, "cannot create pipe" );
		}
		genDirList(&req);
	}


	/* simple response */
	if (req.flag_simple) {
		switch (req.method) {
			case MTHD_GET:
				sendResource(incoming_sock, &req);
				break;
			case MTHD_HEAD:
				sendResource(incoming_sock, &req);
				break;
			case MTHD_POST:
			default:
				req.status=MSG_NOT_IMPL;
				sendResource(incoming_sock, &req);
				break;
		}

	/* full response */
	} else {
		switch (req.method) {
			case MTHD_GET:
				sendRespHeader(incoming_sock, &req);
				sendResource(incoming_sock, &req);
				break;
			case MTHD_HEAD:
				sendRespHeader(incoming_sock, &req);
				break;
			case MTHD_POST:
			default:
				req.status=MSG_NOT_IMPL;
				sendRespHeader(incoming_sock, &req);
				sendResource(incoming_sock, &req);
				break;
		}
	}

	return;
}


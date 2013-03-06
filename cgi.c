#include <err.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "cgi.h"
#include "global.h"


/* getCGI() will parse out the query string and modify path to strictly
 * contain the path of the executable. This way, the calling function can still
 * check the executable file.
 */
void parseCGI(struct http_request *req)
{
	char *path_tok;
	char *query_tok;
	char temp_buf[BUF_SIZE];

	strcpy(temp_buf,req->path);

	path_tok=strtok(temp_buf,"?");
	bzero(req->path,sizeof(req->path));
	strcpy(req->path,path_tok);

	bzero(req->cgi_query,sizeof(req->cgi_query));
	if ( (query_tok=strtok(NULL,"?"))!=NULL) {
		strcpy(req->cgi_query,query_tok);
	}

	return;
}



/* execCGI() executes the CGI. Unfortunately, it has some difficulties in
 * passing a modified environment to the scripts (see README), however, it will
 * still currently execute the scripts.
 */
void execCGI(struct http_request *req)
{
	char *cgi_argv[] = { NULL };
	char *cgi_env[] = { NULL };
	int pid;
	char buf[BUF_SIZE];

	bzero(buf,sizeof(buf));
	sprintf(buf,"QUERY_STRING=%s",req->cgi_query);

	cgi_argv[0] = req->path;
	/*cgi_env[0] = req->cgi_query;*/

	if ( (pid=fork())<0 ) {
		err(EXIT_FAILURE, "fork() error");
		
	/* child process will run execve() */
	} else if (pid==0) {

		close(req->pipefd[0]);  /* close reading from pipe 1 */
		dup2(req->pipefd[1],STDOUT_FILENO);
		execve(req->path,cgi_argv,cgi_env);

		/* should not get here */
		exit(EXIT_FAILURE);
	}

	/* back in the parent */
	close(req->pipefd[1]);  /* close writing to pipe 1 */

	return;
}

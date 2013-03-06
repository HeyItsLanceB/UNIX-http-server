
#include <sys/socket.h>

#include <arpa/inet.h>
#include <netinet/in.h>

#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "global.h"
#include "response.h"
#include "logging.h"


void logAddress(struct http_request *req, int sock)
{
	char client_ip[INET6_ADDRSTRLEN];
	struct sockaddr_storage client;
	socklen_t client_length;

	if ( flag_debug_mode || flag_log ) {
		client_length = sizeof(client);
		getpeername(sock,(struct sockaddr *)&client,&client_length);

		if (client.ss_family == AF_INET) {
			struct sockaddr_in *client_in4 = (struct sockaddr_in *)&client;
			if (inet_ntop(AF_INET, &client_in4->sin_addr, client_ip, sizeof client_ip)<0) {
				err(EXIT_FAILURE,"inet_ntop() error");
			}
		} else if (client.ss_family == AF_INET6) {
			struct sockaddr_in6 *client_in6 = (struct sockaddr_in6 *)&client;
			if (inet_ntop(AF_INET6, &client_in6->sin6_addr, client_ip, sizeof client_ip)<0) {
				err(EXIT_FAILURE,"inet_ntop() error");
			}
		}

		bzero(req->log,sizeof(req->log));
		sprintf(req->log,"%s",client_ip);
	}
	return;
}

void logTime(struct http_request *req)
{
	time_t now;
	if ( flag_debug_mode || flag_log ) {
		if ( (time(&now))<0 ) {
			err(EXIT_FAILURE,"cannot log time");
		}
		strcat(req->log," ");
		addTime(req->log,&now);
	}
	return;
}

void logRequest(struct http_request *req, char *request)
{
	if ( flag_debug_mode || flag_log ) {
		strcat(req->log," \"");
		strcat(req->log, request);
		strcat(req->log,"\"");
	}
	return;
}

void logStatus(struct http_request *req)
{
	char temp[LOG_SIZE];
	if ( flag_debug_mode || flag_log ) {
		sprintf(temp," %d %s",req->status,getStatusStr(req->status));
		strcat(req->log,temp);
	}
	return;
}

void logSize(struct http_request *req, int size)
{
	char temp[LOG_SIZE];
	if ( flag_debug_mode || flag_log ) {
		sprintf(temp," %d",size);
		strcat(req->log,temp);
	}

	/* last bit of log, so write the log */
	sendLog(req);

	return;
}

void sendLog(struct http_request *req)
{
	if ( flag_log ) {
		strcat(req->log,"\n");
		if ( (write(log_file, req->log, strlen(req->log)))<0 ) {
			err(EXIT_FAILURE,"failed to write log");
		}
	} else if ( flag_debug_mode ) {
		printf("%s\n",req->log);
	}
	bzero(req->log,sizeof(req->log));
	return;
}


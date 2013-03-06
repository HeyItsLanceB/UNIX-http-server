/*
 * global.h
 *
 *  Created on: Dec 2, 2012
 *      Author: lance
 */

#ifndef GLOBAL_H_
#define GLOBAL_H_

#include <limits.h>

#define BUF_SIZE (PATH_MAX+4096)
#define LOG_SIZE 2048
#define MAX_CONN 5

#define MSG_OK        200
#define MSG_NO_MOD    304
#define MSG_BAD_REQ   400
#define MSG_NOT_FOUND 404
#define MSG_NOT_IMPL  501

#define MTHD_GET  0
#define MTHD_HEAD 1
#define MTHD_POST 2

#define CGI_REQ  0
#define USER_REQ 1
#define STD_REQ  2

#define CGI_LOC "/cgi-bin/"
#define USER_LOC "sws/"

/* global values initiated at startup */
int listen_port;
int log_file;
char serv_path[PATH_MAX];
char cgi_path[PATH_MAX];

/* flags initiated at startup */
int flag_allow_cgi;
int flag_bind_spec;
int flag_debug_mode;
int flag_log;

/* request structure, holding variables per each request */
struct http_request {
	int status;
	int method;
	int type;
	int flag_dir_list;
	int flag_simple;
	int flag_if_mod;
	int content_size;
	time_t if_time;
	char path[PATH_MAX];
	char log[LOG_SIZE];
	char cgi_query[BUF_SIZE];
	int pipefd[2];
};

#endif /* GLOBAL_H_ */

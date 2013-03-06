/*
 * Lance Burgo
 * 
 * sws - simple web server
 *
 */

#ifdef __NetBSD__
	#include <stdlib.h>
	#include <netinet/in.h>
#endif

#ifdef __linux__
	#include <bsd/stdlib.h>
	#include <netinet/ip.h>
#endif

#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

#include <arpa/inet.h>
#include <err.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "global.h"

#include "daemon.h"
#include "incoming.h"
#include "logging.h"
#include "sighandle.h"
#include "usage.h"


void checkDirPath(char *);


int main(int argc, char **argv)
{
	char option;
	struct sockaddr_in ip4_addr;
	struct sockaddr_in6 ip6_addr;
	int listen_sock4, listen_sock6, listen_sock;
	int incoming_sock;
	int pid, in_ret;
	struct sigaction sig_act;

	int flag_user_addr=0;
	int flag_inet4_only=0;
	int flag_inet6_only=0;

	sig_act.sa_handler = handleSigChld;
	sigemptyset(&sig_act.sa_mask);
	sig_act.sa_flags = SA_RESTART;

	if ( sigaction(SIGCHLD, &sig_act, NULL)<0 ) {
		err(EXIT_FAILURE,"sigaction()");
	}

	setprogname(argv[0]);

	/* set defaults */
	flag_allow_cgi   = 0;
	flag_debug_mode  = 0;
	flag_bind_spec   = 0;
	flag_log         = 0;
	listen_port      = 8080;

	/* parse options */
	while( (option=getopt(argc, argv, "c:dhi:l:p:"))!=-1 )
	{
		switch (option) {
			case 'c':
				flag_allow_cgi=1;
				strcpy(cgi_path,optarg);
				checkDirPath(cgi_path);
				break;
			case 'd':
				flag_debug_mode=1;
				break;
			case 'h':
				usage();
				break;
			case 'i':
				flag_user_addr=1;
				in_ret=inet_pton(AF_INET,optarg,&ip4_addr.sin_addr);
				if (in_ret==0) {
					flag_inet4_only=0;
					flag_inet6_only=1;
				} else if (in_ret==1) {
					flag_inet4_only=1;
					flag_inet6_only=0;
				}
				if (flag_inet6_only) {
					in_ret=inet_pton(AF_INET6,optarg,&ip6_addr.sin6_addr);
				}
				if (in_ret<=0) {
					if (in_ret<0) {
						err(EXIT_FAILURE,"proglem with inet_pton()");
					}
					usage();
					exit(EXIT_FAILURE);
				}
				break;
			case 'l':
				flag_log = 1;
				if ( (log_file=open( optarg,
				         O_WRONLY|O_APPEND|O_CREAT,
				         S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH))<0) {
					err(EXIT_FAILURE,"cannot open log file");
				}
				break;
			case 'p':
				listen_port = strtol(optarg, (char **) NULL, 10);
				if ( (listen_port>65535) || (listen_port<1) ) {
					printf("port out of range");
					usage();
					exit(EXIT_FAILURE);
				}
				break;
			default:
				usage();
				exit (EXIT_FAILURE);
		}
	}


	/* Check the number of arguments passed */
	if ( (argv[optind]==NULL) || (optind+1!=argc) ) {
		printf("Request not understood\n");
		usage();
		exit(EXIT_FAILURE);
	}

	/* check serv_path and move to root directory (primarily for daemon) */
	strcpy(serv_path,argv[optind]);
	checkDirPath(serv_path);
	if ( chdir("/")<0 ) {
		err(EXIT_FAILURE,"unable to change dir");
	}


	if (!flag_user_addr) {
		ip6_addr.sin6_addr = in6addr_any;
	}

	/* For user chosen IPV4 */
	if (flag_inet4_only) {

		if ( (listen_sock4=socket(AF_INET, SOCK_STREAM,0))<0 ) {
			err(EXIT_FAILURE, "cannot open socket");
		}

		ip4_addr.sin_family = AF_INET;
		ip4_addr.sin_port = htons(listen_port);

		if (bind(listen_sock4,(struct sockaddr *)&ip4_addr,sizeof(ip4_addr))<0 ) {
			err(EXIT_FAILURE, "cannot bind socket");
		}

		if ( listen(listen_sock4,MAX_CONN) < 0) {
			err(EXIT_FAILURE, "failed to listen for connections");
		}

	/* For both the default and user chosen IPV6 */
	} else {
	
		if ( (listen_sock6=socket(AF_INET6, SOCK_STREAM,0))<0 ) {
			err(EXIT_FAILURE, "cannot open socket");
		}

		ip6_addr.sin6_family = AF_INET6;
		ip6_addr.sin6_port = htons(listen_port);

		if (bind(listen_sock6,(struct sockaddr *)&ip6_addr,sizeof(ip6_addr))<0 ) {
			err(EXIT_FAILURE, "cannot bind socket");
		}

		if ( listen(listen_sock6,MAX_CONN) < 0) {
			err(EXIT_FAILURE, "failed to listen for connections");
		}
	}

	/* daemonize if not in debug mode */
	if (!flag_debug_mode) {
		daemonize();
	}

	/* debug mode */
	if (flag_debug_mode) {
		for (;;) {

			if ( (flag_user_addr) && (flag_inet4_only) ) {
				listen_sock=listen_sock4;
			} else {
				listen_sock=listen_sock6;
			}

			if ( (incoming_sock=accept(listen_sock,0,0))>=0 ) {
					processRequest(incoming_sock);
					if ((close(incoming_sock))<0) {
						err(EXIT_FAILURE,"cannot close()");
					}
			} else if (incoming_sock<0) {
				err(EXIT_FAILURE,"accept() error");
			}

			/* need to change directory back to root in debug mode */
			if ( chdir("/")<0 ) {
				err(EXIT_FAILURE,"unable to change dir");
			}
		}

	/* standard mode */
	} else {
		for (;;) {

			if ( (flag_user_addr) && (flag_inet4_only) ) {
				listen_sock=listen_sock4;
			} else {
				listen_sock=listen_sock6;
			}

			if ( (incoming_sock=accept(listen_sock,0,0))>=0 ) {

				/* child process services request */
				if ((pid=fork())==0) {
					if (close(listen_sock)<0) {
						err(EXIT_FAILURE,"cannot close()");
					}
					processRequest(incoming_sock);
					if ((close(incoming_sock))<0) {
						err(EXIT_FAILURE,"cannot close()");
					}
					exit(EXIT_SUCCESS);

				/* error fork()ing */
				} else if (pid<0) {
					err(EXIT_FAILURE, "fork() error");
				}

			/* back in the parent */
			} else if (incoming_sock<0) {
				err(EXIT_FAILURE,"accept() error");
			}
				
			if ((close(incoming_sock))<0) {
				err(EXIT_FAILURE,"cannot close()");
			}
		}
	}

	/* should never get here */
	close(listen_sock);
	return 0;
}


void checkDirPath(char *path)
{
	struct stat path_stat;
	char temp[PATH_MAX];

	if ( (lstat(path,&path_stat))<0 ) {
		err(EXIT_FAILURE,"cannot stat() file");
	}

	if (!S_ISDIR(path_stat.st_mode)) {
		printf("\"%s\" is not a valid directory\n",path);
		usage();
		exit (EXIT_FAILURE);
	}

	/* find the absolute path of the directory and use that */
	if ( (realpath(path,temp))==NULL ) {
		err(EXIT_FAILURE,"cannot find path to directory \"%s\"",path);
	}

	strcpy(path,temp);

	if ( path[strlen(path)-1]!='/' ) {
		strcat(path,"/");
	}

	return;
}

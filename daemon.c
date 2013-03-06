#include <sys/types.h>
#include <sys/stat.h>

#include <err.h>
#include <fcntl.h>
#include <limits.h>
#include <paths.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "global.h"
#include "daemon.h"


void daemonize()
{

	int pid, fd;

	if ( (pid=fork())<0 ) {
		err(EXIT_FAILURE,"cannot fork()");
	} else if (pid!=0) {
		exit(EXIT_SUCCESS);
	}

	if ( setsid()<0 )
		err(EXIT_FAILURE,"problem with setsid()");

	umask(0);

	if ( (fd=open(_PATH_DEVNULL, O_RDWR))<0 )
		err(EXIT_FAILURE,"cannot open /dev/null");

	if ( dup2(fd,STDIN_FILENO)<0 )
		err(EXIT_FAILURE,"problem with dup2()");
	if ( dup2(fd,STDOUT_FILENO)<0 )
		err(EXIT_FAILURE,"problem with dup2()");
	if ( dup2(fd,STDERR_FILENO)<0 )
		err(EXIT_FAILURE,"problem with dup2()");

	if ( close(fd)<0 )
		err(EXIT_FAILURE,"cannot close()");

	return;
}

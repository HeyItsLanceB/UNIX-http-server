/*
 * Lance Burgo
 * 
 * sws - simple web server
 *
 */

#include <sys/wait.h>
#include <stdio.h>

#include "sighandle.h"


/* This handler will only wait() on the children */
void handleSigChld(int s)
{
	while( waitpid(-1,NULL,WNOHANG)>0 );
}

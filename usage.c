#include <stdio.h>
#include <stdlib.h>

/* usage() will print the usage of sws and promptly exit */
void usage(void)
{
	printf("Usage: sws [OPTION]... DIR\n");
	printf("Start a web server at root directory DIR\n");
	printf("\n");
	printf("Available options\n");
	printf("  -c DIR     Allow execution of CGIs from the given directory\n");
	printf("             (relative to the document root).\n");
	printf("  −d         Enter debugging mode. That is, do not daemonize, only\n");
	printf("             accept one connection at a time and enable logging to\n");
	printf("             stdout.\n");
	printf("  -h         Print this usage summary and exit\n");
	printf("  -i ADDRESS Bind to the given IPv4 or IPv6 address. If not\n");
	printf("             provided, sws will listen on all IPv4 and IPv6\n");
	printf("             addresses on this host.\n");
	printf("  -l FILE    Log all requests to the given ﬁle.\n");
	printf("  -p PORT    Listen on the given port. If not provided, port 8080\n");
	printf("             will be used.\n");
	printf("\n");
	printf("sws will also serve the directory \"sws\" in each user's home\n");
	printf("folder when a request is made that begins with \'~\'\n");
	exit(EXIT_FAILURE);
}


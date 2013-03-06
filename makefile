sws: sws.o daemon.o incoming.o response.o sighandle.o usage.o logging.o cgi.o dlist.o parse.o
	gcc -Wall -g -o sws sws.o daemon.o incoming.o response.o sighandle.o usage.o logging.o cgi.o dlist.o parse.o -lbsd

sws.o: main.c global.h incoming.h sighandle.h daemon.h logging.h cgi.h
	gcc -Wall -g -c -o sws.o main.c -lbsd

usage.o: usage.c usage.h
	gcc -Wall -g -c -o usage.o usage.c

dlist.o: dlist.c dlist.h global.h
	gcc -Wall -g -c -o dlist.o dlist.c

daemon.o: daemon.c daemon.h global.h
	gcc -Wall -g -c -o daemon.o daemon.c

incoming.o: incoming.c incoming.h global.h response.h logging.h cgi.h dlist.h
	gcc -Wall -g -c -o incoming.o incoming.c

parse.o: parse.c incoming.h global.h response.h logging.h cgi.h dlist.h
	gcc -Wall -g -c -o parse.o parse.c

response.o: response.c response.h global.h logging.h cgi.h
	gcc -Wall -g -c -o response.o response.c

sighandle.o: sighandle.c sighandle.h
	gcc -Wall -g -c -o sighandle.o sighandle.c

logging.o: logging.c logging.h global.h response.h
	gcc -Wall -g -c -o logging.o logging.c

cgi.o: cgi.c cgi.h global.h
	gcc -Wall -g -c -o cgi.o cgi.c

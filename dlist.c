#include <sys/stat.h>
#include <sys/types.h>

#include <bsd/string.h>
#include <bsd/stdlib.h>
#include <dirent.h>
#include <err.h>
#include <errno.h>
#include <grp.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "global.h"
#include "dlist.h"


void
genDirList(struct http_request *req)
{
	char   dir[PATH_MAX];
	int    file_count, i;
	struct file_st *file_array;
	struct stat buf;

	if ( (req->path[0]!='.') && (req->path[0]!='/') ) {
		strcpy(dir,"./");
	} else if (req->path[0]=='/') {
		strcpy(dir,".");
	}

	strcat(dir,req->path);

	if (lstat(dir,&buf)<0) {
		err(EXIT_FAILURE, "cannot access %s", dir);
	}

	if((file_count = getList(dir, &file_array))<1) {
		return;
	}

	sortList(dir, &file_array, file_count);
	genListings(&file_array, file_count);
	printListings(&file_array, file_count, req);

	/* free memory and potentially make a recursive call */
	for (i=0; i<file_count; i++) {
        free(file_array[i].file_name);
        free(file_array[i].full_path);
		free(file_array[i].listing);
	}

	free(file_array);
	return;
}


int
fileFilter(char *file_name)
{
	if (strncmp(file_name, ".", 1)==0) {
		return 0;    /* fail - hidden files start with '.' */
	} else {
		return 1;
	}
}


void
genListings(struct file_st **file_array_ptr, int count)
{
	struct file_st *farr = *file_array_ptr;
	char **file_listing, tempstr[PATH_MAX];
	int i, j;

	if ((file_listing = malloc(count*sizeof(char *)))==NULL) {
		err(EXIT_FAILURE, "out of memory");
	}

	for (i=0; i<count; i++)
	{
		if ((file_listing[i]=malloc(sizeof(char)*PATH_MAX))==NULL) {
			err(EXIT_FAILURE, "out of memory");
		}
		strcpy(file_listing[i],"\0");
	}


	for (i=0; i<count; i++)
	{
		strcpy(tempstr, farr[i].file_name);
		for (j=0; j<strlen(tempstr); j++)
		{
			if ( tempstr[j]<32 || tempstr[j]>126 ) {
				tempstr[j] = '?';
			}
		}

		strcat(file_listing[i],tempstr);
	}
	


	/* Finally, allocate memory and add the listing to the actual
	 * file structure.
	 */
	for (i=0; i<count; i++)
	{
		if ((farr[i].listing
			=malloc((strlen(file_listing[i])+1)*sizeof(char)))
				==NULL) {
			err(EXIT_FAILURE, "out of memory");
		}

		strcpy(farr[i].listing, file_listing[i]);
		free(file_listing[i]);
	}

	return;
}


int
getList(char *dir_name, struct file_st **file_array_ptr)
{
	int first_count=0;
	int count=0;
	DIR *dp;
	struct dirent *dirp;
	struct file_st *file_array;

	if ((dp = opendir(dir_name)) == NULL) {
		warn("cannot open directory %s",dir_name);
		return -1;
	}
	while ( (dirp=readdir(dp)) != NULL) {
		if ((fileFilter(dirp->d_name))==0) {
			continue;
		}
		first_count++;
	}

	if ((closedir(dp))<0) {
		err(EXIT_FAILURE,"cannot close directory %s",dir_name);
	}

	if ((*file_array_ptr=malloc(first_count*sizeof(struct file_st)))
					==NULL) {
			err(EXIT_FAILURE, "out of memory");
	}
	file_array = *file_array_ptr;


	if ((dp = opendir(dir_name)) == NULL) {
		warn("cannot open directory %s",dir_name);
		return -1;
	}
	while ( (dirp=readdir(dp)) != NULL) {
		if ((fileFilter(dirp->d_name))==0) {
			continue;
		}
		if ((file_array[count].file_name
			=malloc((strlen(dirp->d_name)+1)*sizeof(char)))==NULL) {
			err(EXIT_FAILURE, "out of memory");
		}
		strcpy(file_array[count].file_name,dirp->d_name);
		if ( (count+1) > first_count ) {
			break;
		}
		count++;
	}

	if ((closedir(dp))<0) {
		err(EXIT_FAILURE,"cannot close directory %s",dir_name);
	}

	return count;
}


int
masterCmp(const void *fs1, const void *fs2)
{
	int return_val;
	struct file_st *filest1 = (struct file_st *) fs1;
	struct file_st *filest2 = (struct file_st *) fs2;

	return_val = strcasecmp(filest1->file_name, filest2->file_name);
	return return_val;
}


void
printListings(struct file_st **file_array_ptr, int count, struct http_request *req)
{
	struct file_st *farr = *file_array_ptr;
	char write_buf[BUF_SIZE];
	int i, bytes_sent;

	req->content_size=0;

	strcpy(write_buf,"<!DOCTYPE HTML>\n");
	strcat(write_buf,"<html>\n");
	strcat(write_buf,"<head>\n");
	strcat(write_buf,"<title>ERROR</title>\n");
	strcat(write_buf,"</head>\n");
	strcat(write_buf,"<body>\n");
	strcat(write_buf,"<h1>Index</h1>\n");
	strcat(write_buf,"<hr />\n");

	if ( (bytes_sent=write( req->pipefd[1],
	                        write_buf,
	                        strlen(write_buf))) < 0 ) {
		err(EXIT_FAILURE,"cannot write to pipe");
	}

	req->content_size+=bytes_sent;

	if ( strcmp(req->path,"/")!=0 ) {
		bzero(write_buf,sizeof(write_buf));
		sprintf(write_buf,"<p><a href = \"../\">parent</a></p>\n");
		strcat(write_buf,"<hr />\n");

		if ( (bytes_sent=write( req->pipefd[1],
								write_buf,
								strlen(write_buf))) < 0 ) {
			err(EXIT_FAILURE,"cannot write to pipe");
		}
		req->content_size+=bytes_sent;
	}


	for (i=0; i<count; i++)
	{
		bzero(write_buf,sizeof(write_buf));
		sprintf(  write_buf,
		          "<p><a href = \"%s\">%s</a></p>\n",
		          farr[i].listing,
		          farr[i].listing  );

		if ( (bytes_sent=write( req->pipefd[1],
								write_buf,
								strlen(write_buf))) < 0 ) {
			err(EXIT_FAILURE,"cannot write to pipe");
		}
		req->content_size+=bytes_sent;
	}

	bzero(write_buf,sizeof(write_buf));
	strcpy(write_buf,"</body>\n");
	strcat(write_buf,"</html>\n");
	if ( (bytes_sent=write( req->pipefd[1],
	                        write_buf,
	                        strlen(write_buf))) < 0 ) {
		err(EXIT_FAILURE,"cannot write to pipe");
	}
	req->content_size+=bytes_sent;

	if ( close(req->pipefd[1]) < 0 ) {
		err(EXIT_FAILURE,"cannot close pipe");
	}


	return;
}


void
sortList(char *dir, struct file_st **file_array_ptr, int count)
{
	int i;
	struct file_st *file_array = *file_array_ptr;
	char temp[PATH_MAX];

	for (i=0; i<count; i++)
	{
		if ( dir[strlen(dir)-1] == '/') {
			strcpy(temp,dir);
			strcat(temp,file_array[i].file_name);
		} else {
			strcpy(temp,dir);
			strcat(temp, "/");
			strcat(temp,file_array[i].file_name);
		}
		if ((file_array[i].full_path
					=malloc((strlen(temp)+1)*sizeof(char)))
						==NULL) {
			err(EXIT_FAILURE, "out of memory");
		}
		strcpy(file_array[i].full_path,temp);
		if ((lstat(file_array[i].full_path,
						&file_array[i].file_stat))<0) {
			warn("cannot stat %s",file_array[i].full_path);
		}
	}

	qsort(file_array, count, sizeof(struct file_st), masterCmp);

	return;
}


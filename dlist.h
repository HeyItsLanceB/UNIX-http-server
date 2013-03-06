/*
 * dlist.h
 *
 *  Created on: Dec 8, 2012
 *      Author: lance
 */

#ifndef DLIST_H_
#define DLIST_H_

#include "global.h"


struct file_st {
	char *full_path;
	char *file_name;
	char *listing;
	struct stat file_stat;
};

void  genDirList(struct http_request *);
void  accessDir(char *);
int   fileFilter(char *file_name);
void  genListings(struct file_st**, int);
int   getList(char *, struct file_st**);
int   masterCmp(const void *fs1, const void *fs2);
void  printListings(struct file_st**, int, struct http_request *);
void  sortList(char *, struct file_st**, int);

#endif /* DLIST_H_ */

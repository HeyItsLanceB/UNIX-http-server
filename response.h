/*
 * response.h
 *
 *  Created on: Dec 2, 2012
 *      Author: lance
 */

#ifndef RESPONSE_H_
#define RESPONSE_H_

#define ERR_MSG_LEN 139

#include "global.h"

void addTime(char *, time_t *);
char *getStatusStr(int);
void sendResource(int, struct http_request *);
void sendRespHeader(int, struct http_request *);

#endif /* RESPONSE_H_ */

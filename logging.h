/*
 * logging.h
 *
 *  Created on: Dec 8, 2012
 *      Author: lance
 */

#ifndef LOGGING_H_ 
#define LOGGING_H_

#include "global.h"

void logAddress(struct http_request *, int);
void logTime(struct http_request *);
void logRequest(struct http_request *, char *);
void logStatus(struct http_request *);
void logSize(struct http_request *, int);
void sendLog(struct http_request *);

#endif /* LOGGING_H_ */

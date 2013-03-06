/*
 * cgi.h
 *
 *  Created on: Dec 12, 2012
 *      Author: lance
 */

#ifndef CGI_H_
#define CGI_H_

#include "global.h"

void parseCGI(struct http_request *req);
void execCGI(struct http_request *req);

#endif /* CGI_H_ */

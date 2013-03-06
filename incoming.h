/*
 * incoming.h
 *
 *  Created on: Dec 2, 2012
 *      Author: lance
 */

#ifndef INCOMING_H_
#define INCOMING_H_

#include "global.h"

void parseIncoming(int, struct http_request *);
void processRequest(int);

#endif /* INCOMING_H_ */

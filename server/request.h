#ifndef _REQUEST_H
#define _REQUEST_H

/**
 * Read incoming server requests from given socket descriptor
 */
void *parse_request(void *arg);

#endif
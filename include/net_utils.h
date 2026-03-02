#ifndef NET_UTILS_H
#define NET_UTILS_H

#include <netinet/in.h>

#define PORT_NO 8080
#define BACKLOG_SIZE 1024

/* return listening socket FD */
int create_lstnsock(in_port_t port, int backlog);

#endif

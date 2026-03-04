#ifndef NET_UTILS_H
#define NET_UTILS_H

#include "message.h"
#include <netinet/in.h>

#define PORT_NO 8080
#define BACKLOG_SIZE 1024

/* return listening socket FD */
int create_lstnsock(in_port_t port, int backlog);

int clntsock_arr_add(int sockfd);
int clntsock_arr_del(int sockfd);
void clntsock_broadcast(msg_data *md);

#endif

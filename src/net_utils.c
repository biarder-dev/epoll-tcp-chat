#include "net_utils.h"
#include <netinet/in.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static void set_servaddr(struct sockaddr_in *addr, in_port_t port)
{
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);
    addr->sin_addr.s_addr = INADDR_ANY;
}

int create_lstnsock(in_port_t port, int backlog)
{
    int sockfd, optval;
    socklen_t addrlen;
    struct sockaddr_in servaddr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) goto err_exit;

    optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));

    addrlen = sizeof(servaddr);
    memset(&servaddr, 0, addrlen);
    set_servaddr(&servaddr, port);
    if (bind(sockfd, (struct sockaddr *)&servaddr, addrlen) < 0)
        goto close_sock;
    if (listen(sockfd, backlog) < 0) goto close_sock;
    return sockfd;

close_sock:
    close(sockfd);
err_exit:
    return -1;
}

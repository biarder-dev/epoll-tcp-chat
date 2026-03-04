#include "net_utils.h"
#include "message.h"
#include <netinet/in.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

static pthread_rwlock_t rwlock = PTHREAD_RWLOCK_INITIALIZER;
static int *clntsock_arr = NULL;
static int sock_count, max_sockets = 0;

static int comp_int(const void *a, const void *b)
{
    int _a, _b;
    _a = *(int *)a;
    _b = *(int *)b;

    if (_a > _b) return 1;
    if (_a < _b) return -1;
    return 0;
}

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

int clntsock_arr_add(int sockfd)
{
    int result, *tmp;
    enum { INIT = 1, GROW = 2 };
    pthread_rwlock_wrlock(&rwlock);

    result = 0;
    if (clntsock_arr == NULL) {
        clntsock_arr = (int *)malloc(sizeof(int) * INIT);

        if (clntsock_arr == NULL) {
            result = -1;
        } else {
            clntsock_arr[sock_count++] = sockfd;
            max_sockets = INIT;
        }
    } else if (sock_count >= max_sockets) {
        tmp = (int *)realloc(clntsock_arr, sizeof(int) * (sock_count * GROW));

        if (tmp == NULL) {
            result = -1;
        } else {
            clntsock_arr = tmp;
            clntsock_arr[sock_count++] = sockfd;
            max_sockets *= GROW;
        }
    } else {
        clntsock_arr[sock_count++] = sockfd;
    }

    pthread_rwlock_unlock(&rwlock);
    return result;
}

int clntsock_arr_del(int sockfd)
{
    int *target;
    ptrdiff_t idx;
    pthread_rwlock_wrlock(&rwlock);

    if (sock_count <= 0) {
        pthread_rwlock_unlock(&rwlock);
        return -1;
    }

    qsort(clntsock_arr, sock_count, sizeof(*clntsock_arr), comp_int);
    target = bsearch(&sockfd, clntsock_arr, sock_count, sizeof(*clntsock_arr),
                     comp_int);
    if (target == NULL) {
        pthread_rwlock_unlock(&rwlock);
        return -1;
    }

    idx = target - clntsock_arr;
    if (idx < sock_count - 1)
        memmove(clntsock_arr + idx, clntsock_arr + (idx + 1),
                sizeof(*clntsock_arr) * (sock_count - (idx + 1)));

    sock_count--;
    pthread_rwlock_unlock(&rwlock);
    return 0;
}

void clntsock_broadcast(msg_data *md)
{
    int i;
    char buffer[1024];

    snprintf(buffer, sizeof(buffer), "%s: %s\n", md->author->nickname, md->msg);
    pthread_rwlock_rdlock(&rwlock);

    /* O_NONBLOCK setted at accepting */
    for (i = 0; i < sock_count; i++) {
        write(clntsock_arr[i], buffer, strlen(buffer));
    }

    pthread_rwlock_unlock(&rwlock);
}

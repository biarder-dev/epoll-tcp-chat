#include "worker.h"
#include "net_utils.h"
#include <asm-generic/errno-base.h>
#include <asm-generic/errno.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_EVENTS 1024

static int set_nonblocking(int fd)
{
    int flags;
    if ((flags = fcntl(fd, F_GETFL, 0)) < 0) return -1;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

static void setev(struct epoll_event *ev, int sockfd, int flags)
{
    ev->data.fd = sockfd;
    ev->events = flags;
}

void *start_worker(void *arg)
{
    int i, epfd, nfd;
    int lstn_sockfd, clnt_sockfd;
    socklen_t addrlen;
    struct sockaddr_in clntaddr;
    struct epoll_event ev, events[MAX_EVENTS];

    if ((lstn_sockfd = create_lstnsock(PORT_NO, BACKLOG_SIZE)) < 0) return NULL;
    set_nonblocking(lstn_sockfd);

    if ((epfd = epoll_create1(0)) < 0) goto cleanup;

    setev(&ev, lstn_sockfd, EPOLLIN | EPOLLET);
    if (epoll_ctl(epfd, EPOLL_CTL_ADD, lstn_sockfd, &ev) < 0) goto cleanup;

    for (;;) {
        nfd = epoll_wait(epfd, events, MAX_EVENTS, -1);
        if (nfd < 0) {
            if (errno == EINTR) continue;
            /* Log error */
            goto cleanup;
        }

        for (i = 0; i < nfd; i++) {
            if ((events[i].events & EPOLLERR) ||
                (events[i].events & EPOLLHUP)) {
                close(events[i].data.fd);
                continue;
            }

            if (events[i].data.fd == lstn_sockfd) {
                for (;;) {
                    addrlen = sizeof(clntaddr);
                    clnt_sockfd = accept(
                        lstn_sockfd, (struct sockaddr *)&clntaddr, &addrlen);
                    if (clnt_sockfd < 0) {
                        if (errno == EINTR) continue;
                        if (errno == EAGAIN || errno == EWOULDBLOCK) break;

                        /* Log error */
                        goto cleanup;
                    }

                    set_nonblocking(clnt_sockfd);
                    setev(&ev, clnt_sockfd, EPOLLIN | EPOLLET);
                    if (epoll_ctl(epfd, EPOLL_CTL_ADD, clnt_sockfd, &ev) < 0)
                        close(clnt_sockfd);
                }
            } else {
                close(events[i].data.fd);
            }
        }
    }
    return arg;

cleanup:
    close(lstn_sockfd);
    close(epfd);
    return NULL;
}

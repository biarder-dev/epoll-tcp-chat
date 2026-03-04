#include "worker.h"
#include "message.h"
#include "net_utils.h"
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_EVENTS 1024
#define NOT_IMPL -1 /* Temporary variable */

static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t q_mtx = PTHREAD_MUTEX_INITIALIZER;

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
    usr_data ud;
    char buf[1024];
    int epfd, nfd, i, n;
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
                    clntsock_arr_add(clnt_sockfd);

                    setud(&ud, "HOST", INADDR_ANY);
                    snprintf(buf, sizeof(buf), "%s is joined!",
                             inet_ntoa(clntaddr.sin_addr));

                    pthread_mutex_lock(&q_mtx);
                    msg_enque(buf, &ud);
                    pthread_cond_signal(&cond);
                    pthread_mutex_unlock(&q_mtx);
                }
            } else {
                for (;;) {
                    n = read(events[i].data.fd, buf, sizeof(buf) - 1);
                    if (n < 0) {
                        if (errno == EINTR) continue;
                        if (errno == EAGAIN || errno == EWOULDBLOCK) break;

                        /* Log error */
                        goto cleanup;
                    } else if (n == 0) {
                        close(events[i].data.fd);
                        clntsock_arr_del(events[i].data.fd);
                    } else {
                        buf[n] = '\0';
                        setud(&ud, "Anonymous", NOT_IMPL);
                        pthread_mutex_lock(&q_mtx);
                        msg_enque(buf, &ud);
                        pthread_cond_signal(&cond);
                        pthread_mutex_unlock(&q_mtx);
                    }
                }
            }
        }
    }
    return arg;

cleanup:
    close(lstn_sockfd);
    close(epfd);
    return NULL;
}

void *start_broadcaster(void *arg)
{
    msg_data *md;
    for (;;) {
        pthread_mutex_lock(&q_mtx);
        while ((md = msg_deque()) == NULL)
            pthread_cond_wait(&cond, &q_mtx);
        pthread_mutex_unlock(&q_mtx);

        clntsock_broadcast(md);
        freemd(md);
    }

    return arg;
}

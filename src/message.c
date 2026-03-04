#include "message.h"
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

static msg_data *lowest, *highest = NULL;

void msg_enque(char *msg, usr_data *ud)
{
    char *msgcpy;
    usr_data *udcpy;

    msgcpy = strdup(msg);
    udcpy = (usr_data *)malloc(sizeof(usr_data));
    udcpy->nickname = strdup(ud->nickname);
    udcpy->ip_addr = ud->ip_addr;

    msg_data *curr;
    curr = (msg_data *)malloc(sizeof(msg_data));
    curr->msg = msgcpy;
    curr->author = udcpy;
    curr->next = NULL;
    curr->prev = NULL;

    if (lowest == NULL) {
        lowest = highest = curr;
    } else {
        curr->next = highest;
        highest->prev = curr;
        highest = curr;
    }
}

msg_data *msg_deque()
{
    msg_data *low;
    if (lowest == NULL) return NULL;

    low = lowest;
    lowest = lowest->prev;

    if (lowest == NULL) highest = NULL;
    else
        lowest->next = NULL;

    return low;
}

int setud(usr_data *ud, char *nickname, in_addr_t addr)
{
    if (ud == NULL) return -1;
    ud->nickname = nickname;
    ud->ip_addr = addr;
    return 0;
}

int freemd(msg_data *md)
{
    if (md == NULL) return -1;
    if (md->author) {
        if (md->author->nickname) free(md->author->nickname);
        free(md->author);
    }
    if (md->msg) {
        free(md->msg);
    }
    free(md);
    return 0;
}

#ifndef MESSAGE_H
#define MESSAGE_H

#include <netinet/in.h>

typedef struct usr_data usr_data;
struct usr_data {
    char *nickname;
    in_addr_t ip_addr;
};

typedef struct msg_data msg_data;
struct msg_data {
    char *msg;
    usr_data *author;
    msg_data *next;
    msg_data *prev;
};

void msg_enque(char *msg, usr_data *ud);
msg_data *msg_deque();
int setud(usr_data *ud, char *nickname, in_addr_t addr);
int freemd(msg_data *md);

#endif

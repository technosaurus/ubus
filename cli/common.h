#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <libgen.h>

int mkpath(const char *s, mode_t mode);
int mksocket(const char *s);
char * ubus_escaped_arg(const char * a, int len, int * outlen);
int ubus_next_arg(const char * in,  char * out, int size, int * offset);

// list of connected clients
struct  client_list_el{
    char * buff;
    int bufflen;
    int fd;
    struct client_list_el * next;
};

typedef struct client_list_el client;
client * clients;

void client_add(int fd){
    client * c=(client *)malloc(sizeof(client));
    c->fd=fd;
    c->buff=0;
    c->bufflen=0;
    c->next=NULL;
    if(clients==NULL){
        clients=c;
        return;
    }
    client * cur=clients;
    while(cur){
        if(cur->next==NULL){
            cur->next=c;
            return;
        }
        cur=cur->next;
    }
    fprintf(stderr,"corrupted linked list");
    abort();
}
void client_del(int fd){
    client * prev=NULL;
    client * cur=clients;
    while(cur){
        if(cur->fd==fd){
            if(prev){
                prev->next=cur->next;
            }else{
                clients=cur->next;
            }
            if(cur->buff)
                free(cur->buff);
            free(cur);
            return;
        }
        prev=cur;
        cur=cur->next;
    }
    fprintf(stderr,"corrupted linked list");
    abort();
}

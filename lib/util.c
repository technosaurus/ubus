#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <libgen.h>


static int mkpath(const char *s, mode_t mode){
    char *q, *r = NULL, *path = NULL, *up = NULL;
    int rv;

    rv = -1;
    if (strcmp(s, ".") == 0 || strcmp(s, "/") == 0)
        return (0);

    if ((path = strdup(s)) == NULL)
        exit(1);

    if ((q = strdup(s)) == NULL)
        exit(1);

    if ((r = dirname(q)) == NULL)
        goto out;

    if ((up = strdup(r)) == NULL)
        exit(1);

    if ((mkpath(up, mode) == -1) && (errno != EEXIST))
        goto out;

    if ((mkdir(path, mode) == -1) && (errno != EEXIST))
        rv = -1;
    else
        rv = 0;

 out:
    if (up != NULL)
        free(up);
    free(q);
    free(path);
    return (rv);
}

static int mksocketpath(const char *s){
    char * tmp=malloc(strlen(s));
    strcpy(tmp,s);
    int r=mkpath(dirname(tmp),S_IRWXU|S_IRWXG);
    free(tmp);
    return r;
}

static ubus_channel * ubus_client_add(ubus_service * service, ubus_channel * c){
    c->next=NULL;
    c->prev=NULL;
    if(service->chanlist==NULL){
        service->chanlist=c;
        return c;
    }
    ubus_channel * cur=service->chanlist;
    while(cur){
        if(cur->next==NULL){
            cur->next=c;
            c->prev=cur;
            return c;
        }
        cur=cur->next;
    }
    fprintf(stderr,"corrupted linked list");
    abort();
}
static ubus_channel *  ubus_client_del(ubus_service * service, ubus_channel  * c){
    if(service!=c->service){
        fprintf(stderr,"eerr ... trying to delete channel from foreign service. something broke.");
        abort();
    }
    ubus_channel * n= c->next;
    if(c->prev==NULL){
        if(c!=service->chanlist){
            fprintf(stderr,"corrupted linked list");
            abort;
        }
        service->chanlist=0;
    }else{
        c->prev->next=c->next;
    }
    c->service=0;
    ubus_disconnect(c);
    return n;
}

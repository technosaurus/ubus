#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <libgen.h>

#include "ubus.h"
#include "ubus-util.h"


int mkpath(const char *s, mode_t mode){
    char *q = NULL, *r = NULL, *path = NULL, *up = NULL;
    int rv;

    rv = 1;
    if (!strcmp(s, ".") || !strcmp(s, "/"))
        return 0;
    if ((path = strdup(s)) == NULL)
        return 1;
    if ((q = strdup(s)) == NULL)
        goto out;
    if ((r = dirname(q)) == NULL)
        goto out;
    if ((up = strdup(r)) == NULL)
        goto out;
    if ((mkpath(up, mode) == -1) && (errno != EEXIST))
        goto out;
    if ((mkdir(path, mode) == -1) && (errno != EEXIST))
        goto out;
    rv = 0;

out:
    if (up != NULL)
        free(up);
    if (q != NULL)
        free(q);
    if (path != NULL)
        free(path);
    return rv;
}

int mksocketpath(const char *s) {
    char *tmp = strdup(s);
    char *tmpt = tmp;
    tmpt = dirname(tmpt);
    int r = mkpath(tmpt, S_IRWXU|S_IRWXG);
    free(tmp);

    return r;
}

// #define DEBUG_LINKED_LIST 1

ubus_channel *ubus_client_add(ubus_service *service, ubus_channel *c) {
    ubus_channel *cur;

    c->next = NULL;
    c->prev = NULL;
    if(service->chanlist == NULL){
        service->chanlist = c;
        return c;
    }

    for (cur = service->chanlist; cur; cur = cur->next) {
        if (cur->next == NULL){
            cur->next = c;
            c->prev = cur;
            return c;
        }
    }

    return NULL;
}

ubus_channel *ubus_client_del(ubus_service *service, ubus_channel *c) {
    ubus_channel *n;

    if (c == 0)
        return NULL;

#if DEBUG_LINKED_LIST
    fprintf(stderr, "pre (deleting %p) \n", c);
    for (ubus_channel *e = service->chanlist; e; e =  e->next) {
        fprintf(stderr, "  %p\n", e);
    }
#endif

    if (service != c->service)
        return NULL;

    ubus_channel *prev = c->prev;
    ubus_channel *next = c->next;

    if (prev)
        prev->next = next;

    if (next)
        next->prev = prev;

    if (!prev) {
        if (c != service->chanlist) {
            fprintf(stderr, "loose element in chanlist  %p\n", c);
            return NULL;
        }
        service->chanlist = next;
    }

    c->service = 0;
    ubus_disconnect(c);


#if DEBUG_LINKED_LIST
    fprintf(stderr, "post\n");
    for (ubus_channel *e = service->chanlist; e; e =  e->next) {
        fprintf(stderr, "  %p\n", e);
    }
#endif


    return n;
}


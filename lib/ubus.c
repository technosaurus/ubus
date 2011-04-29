#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/inotify.h>

#include "ubus.h"


struct ubus_channel_s;
typedef struct ubus_service_s {
    struct ubus_channel_s *chanlist;
    int fd;
    struct sockaddr_un local;
} ubus_service;

typedef struct ubus_channel_s {
    UBUS_STATUS status;
    int fd;
    int inotify;
    ubus_service *service;
    struct sockaddr_un remote;

    //todo: safe bytes, make those flags
    int activated;
    int isnew;

    struct ubus_channel_s *next;
    struct ubus_channel_s *prev;
} ubus_channel;

#include "util.c"

ubus_t * ubus_create (const char *uri) {
    ubus_service *service = (ubus_service *)malloc(sizeof(ubus_service));
    service->chanlist = NULL;
    if (mksocketpath(uri) < 0){
        return 0;
    }
    if ((service->fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        return 0;
    }
    service->local.sun_family = AF_UNIX;
    strcpy(service->local.sun_path, uri);
    unlink(service->local.sun_path); //FIXME: find better way
    if (bind(service->fd, (struct sockaddr *)&(service->local),
                strlen(service->local.sun_path) + sizeof(service->local.sun_family)) == -1) {
        free(service);
        return 0;
    }
    if (listen(service->fd, 5) == -1) {
        free(service);
        return 0;
    }
    fcntl(service->fd, F_SETFL, fcntl(service->fd, F_GETFL) | O_NONBLOCK);
    return service;
}

int ubus_fd (ubus_t *s) {
    return ((ubus_service*)(s))->fd;
}

ubus_t *ubus_accept (ubus_t *s) {
    ubus_service *server = (ubus_service *)(s);
    ubus_channel *chan = malloc(sizeof(ubus_channel));
    chan->status = UBUS_CONNECTED;
    chan->inotify = 0;
    chan->service = server;
    chan->next = NULL;
    chan->prev = NULL;
    chan->activated = 0;
    chan->isnew = 1;
    uint t = sizeof(chan->remote);
    chan->fd = accept(server->fd, (struct sockaddr *)&(chan->remote), &t);
    if (chan->fd == -1) {
        free(chan);
        return 0;
    } else {
        fcntl(chan->fd, F_SETFL, fcntl(chan->fd, F_GETFL) | O_NONBLOCK);
        ubus_client_add(server, chan);
        return chan;
    }
}
void ubus_destroy (ubus_t *s) {
    ubus_service *server = (ubus_service*)(s);

    ubus_channel *e = server->chanlist;
    while (e) {
        e = ubus_client_del(server, e);
    }
    close(server->fd);
    unlink(server->local.sun_path);
    free(server);
}

//high level bus api
int ubus_broadcast  (ubus_t *s, const void *buff, int len) {
    ubus_service * server = (ubus_service*)(s);
    ubus_channel *e = server->chanlist;
    while (e) {
        ubus_write(e, buff, len);
        e = e->next;
    }
    return len;
}
ubus_chan_t *ubus_ready_chan (ubus_t *s) {
    ubus_service *server= (ubus_service *)(s);
    ubus_channel *e = server->chanlist;
    while (e) {
        if(e->activated) {
            e->activated = 0;
            return e;
        }
        e = e->next;
    }
    return 0;
}

ubus_chan_t *ubus_fresh_chan (ubus_t *s) {
    ubus_service *server = (ubus_service *)(s);
    ubus_channel *e = server->chanlist;
    while (e) {
        if(e->isnew) {
            e->isnew = 0;
            return e;
        }
        e = e->next;
    }
    return 0;
}


#ifndef NO_SELECT
int ubus_select_all (ubus_t *s, fd_set *fds) {
    ubus_service *server= (ubus_service*)(s);

    int fd = ubus_fd(server);
    int max = fd;
    FD_SET(fd, fds);
    ubus_channel *e = server->chanlist;
    while (e) {
        fd = ubus_chan_fd(e);
        FD_SET(fd, fds);
        if (fd>max) {
            max = fd;
        }
        e = e->next;
    }
    return max;
}

void ubus_activate_all (ubus_t *s, fd_set *fds, int flags) {
    ubus_service *server = (ubus_service*)(s);
    ubus_channel *e = server->chanlist;
    while (e) {
        if (FD_ISSET(ubus_chan_fd(e), fds)) {
            ubus_activate(e);
            if (flags & UBUS_IGNORE_INBOUND) {
                static char ignored_buff[1000];
                if (ubus_read(e, &ignored_buff, 1000) < 1) {
                    ubus_disconnect(e);
                }
            } else {
                e->activated = 1;
            }
        }
        e = e->next;
    }
    if (FD_ISSET(ubus_fd(server), fds)) {
        ubus_accept(server);
    }
}
#endif





//chan api

ubus_chan_t *ubus_connect (const char *uri) {
    ubus_channel * chan = malloc(sizeof(ubus_channel));
    chan->status = UBUS_INIT;
    chan->inotify = 0;
    chan->fd = 0;
    chan->service = 0;
    chan->remote.sun_family = AF_UNIX;
    strcpy(chan->remote.sun_path, uri);
    return (ubus_chan_t*)chan;
}

UBUS_STATUS ubus_status  (ubus_chan_t *s) {
    ubus_channel * chan=(ubus_channel*)(s);
    return chan->status;
}
int ubus_chan_fd (ubus_chan_t *s){
    ubus_channel *chan = (ubus_channel*)(s);
    if (chan->inotify!=0) {
        return chan->inotify;
    }
    return chan->fd;
}

static char do_connect(ubus_channel *chan) {
    if ((chan->fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        chan->status = UBUS_ERROR;
        return -1;
    }
    fcntl(chan->fd, F_SETFL, fcntl(chan->fd, F_GETFL) | O_NONBLOCK);
    int len = strlen(chan->remote.sun_path) + sizeof(chan->remote.sun_family);
    if (connect(chan->fd, (struct sockaddr *)&(chan->remote), len) == -1) {
        chan->status = UBUS_ERROR;
        return -1;
    }
    chan->status = UBUS_CONNECTED;
    return 0;
}

UBUS_STATUS ubus_activate (ubus_chan_t *s) {
    ubus_channel *chan = (ubus_channel *)(s);
    if (chan->status == UBUS_INIT) {
        struct stat s;
        if (stat(chan->remote.sun_path,&s)>-1 && do_connect(chan) == 0) {
            if (chan->inotify != 0) {
                close(chan->inotify);
            }
            chan->inotify = 0;
        } else {
            chan->inotify = inotify_init();
            fcntl(chan->inotify, F_SETFL, fcntl(chan->inotify, F_GETFL) | O_NONBLOCK);
            char *tmp = malloc(strlen(chan->remote.sun_path));
            strcpy(tmp, chan->remote.sun_path);
            if (inotify_add_watch(chan->inotify, dirname(tmp), IN_CREATE) < 0){
                chan->status = UBUS_ERROR;
            }
            free(tmp);
            chan->status = UBUS_LURKING;
        }
    } else if(chan->status == UBUS_LURKING) {
        char buff[2000];
        read(chan->inotify, &buff, 2000);
        struct stat s;
        if (stat(chan->remote.sun_path,&s) >- 1) {
            if (do_connect(chan) == 0) {
                close(chan->inotify);
                chan->inotify=0;
            } else {
                /*
                   FIXME
                   (file create && listen) is not atomic
                   avoid dead lock for now by waiting.
                   this should be solved with flock
                   */
                usleep(100);
                if (do_connect(chan) == 0){
                    close(chan->inotify);
                    chan->inotify = 0;
                } else {
                    chan->status = UBUS_LURKING;
                }
            }
        }
    } else if(chan->status == UBUS_CONNECTED) {
        return UBUS_READY;
    }
    fprintf(stderr,"s_%i\n", chan->status);
    return chan->status;
}
int ubus_write (ubus_chan_t *s, const void *buff, int len) {
    ubus_channel * chan = (ubus_channel*)(s);
    int r = send(chan->fd, buff, len, 0);
    fsync(chan->fd);
    return r;
}
int ubus_read (ubus_chan_t *s, void *buff, int len) {
    ubus_channel * chan = (ubus_channel*)(s);
    int r = recv(chan->fd,buff,len,0);
    if (r == 0) {
        chan->status = UBUS_EOF;
    } else if(r < 0) {
        chan->status = UBUS_ERROR;
    } else {
        chan->status = UBUS_CONNECTED;
    }
    return r;
}
void ubus_close (ubus_chan_t *s) {
    ubus_channel * chan = (ubus_channel*)(s);
    shutdown(chan->fd, SHUT_WR);
}
void ubus_disconnect (ubus_chan_t *s){
    ubus_channel * chan = (ubus_channel*)(s);
    if (chan->service) {
        ubus_client_del(chan->service, chan);
    } else {
        if (chan->inotify != 0) {
            close (chan->inotify);
        }
        close(chan->fd);
        free(chan);
    }
}


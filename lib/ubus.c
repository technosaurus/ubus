#include <unistd.h>
#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/inotify.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <libgen.h>

#include "ubus.h"
#include "ubus-util.h"


void *my_alloc(size_t size) {
    return calloc(1, size);
}


ubus_t * ubus_create (const char *uri) {
    ubus_service *service = (ubus_service *)my_alloc(sizeof(ubus_service));

    service->chanlist = NULL;
    if (mksocketpath(uri))
        return NULL;
    if ((service->fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
        return NULL;
    fcntl(service->fd, F_SETFD, fcntl(service->fd, F_GETFD) | FD_CLOEXEC);

    service->local.sun_family = AF_UNIX;
    strcpy(service->local.sun_path, uri);
    unlink(service->local.sun_path); //FIXME: find better way
    if (bind(service->fd, (struct sockaddr *)&(service->local),
                strlen(service->local.sun_path) + sizeof(service->local.sun_family)) < 0) {
        free(service);
        return NULL;
    }
    if (listen(service->fd, 5) < 0) {
        free(service);
        return NULL;
    }
    fcntl(service->fd, F_SETFL, fcntl(service->fd, F_GETFL) | O_NONBLOCK);

    return service;
}

int ubus_fd (ubus_t *s) {
    return ((ubus_service*)(s))->fd;
}

ubus_t *ubus_accept (ubus_t *s) {
    ubus_service *server = (ubus_service *)(s);
    ubus_channel *chan = my_alloc(sizeof(ubus_channel));
    unsigned int t;

    chan->status = UBUS_CONNECTED;
    chan->inotify = 0;
    chan->service = server;
    chan->next = NULL;
    chan->prev = NULL;
    chan->activated = 0;
    chan->isnew = 1;
    t = sizeof(chan->remote);
    chan->fd = accept(server->fd, (struct sockaddr *)&(chan->remote), &t);
    if (chan->fd == -1) {
        free(chan);
        return NULL;
    } else {
        fcntl(chan->fd, F_SETFD, fcntl(chan->fd, F_GETFD) | FD_CLOEXEC);
        fcntl(chan->fd, F_SETFL, fcntl(chan->fd, F_GETFL) | O_NONBLOCK);
        ubus_client_add(server, chan);
    }

    return chan;
}

void ubus_destroy (ubus_t *s) {
    ubus_service *server = (ubus_service*)(s);
    ubus_channel *e;

    for (e = server->chanlist; e; e = ubus_client_del(server, e));

    close(server->fd);
    unlink(server->local.sun_path);
    free(server);
}

//high level bus api
int ubus_broadcast  (ubus_t *s, const void *buff, int len) {
    ubus_channel *e;

    for (e = ((ubus_service *)s)->chanlist; e; e = e->next)
        ubus_write(e, buff, len);

    return len;
}

ubus_chan_t *ubus_ready_chan (ubus_t *s) {
    ubus_channel *e;

    for (e = ((ubus_service *)s)->chanlist; e; e = e->next) {
        if(e->activated) {
            e->activated = 0;
            return e;
        }
    }
    return NULL;
}

ubus_chan_t *ubus_fresh_chan (ubus_t *s) {
    ubus_channel *e = ((ubus_service *)s)->chanlist;

    while (e) {
        if(e->isnew) {
            e->isnew = 0;
            return e;
        }
        e = e->next;
    }

    return NULL;
}


#ifndef NO_SELECT
int ubus_select_all (ubus_t *s, fd_set *fds) {
    ubus_service *server= (ubus_service*)(s);
    ubus_channel *e;
    int fd = ubus_fd(server);
    int max = fd;

    FD_SET(fd, fds);
    for (e = server->chanlist; e; e = e->next) {
        fd = ubus_chan_fd(e);
        FD_SET(fd, fds);
        if (fd>max) {
            max = fd;
        }
    }

    return max;
}

void ubus_activate_all (ubus_t *s, fd_set *fds, int flags) {
    ubus_service *server = (ubus_service*)(s);
    ubus_channel *e;

    for (e = server->chanlist; e;) {
        if (FD_ISSET(ubus_chan_fd(e), fds)) {
            ubus_activate(e);
            if (flags & UBUS_IGNORE_INBOUND) {
                static char ignored_buff[1000];
                int ret = ubus_read(e, &ignored_buff, 1000);
                if (ret < 1) {
                    if (ret < 0 && errno == EAGAIN) {
                        // fd was set for no reason :(
                        e = e->next;
                        continue;
                    }

                    ubus_channel *e2 = e;
                    e = e->next;
                    ubus_disconnect(e2);
                    continue;
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
    ubus_channel* chan = my_alloc(sizeof(ubus_channel));

    if (chan == NULL)
        return NULL;

    chan->status = UBUS_INIT;
    chan->inotify = 0;
    chan->fd = 0;
    chan->service = 0;
    chan->remote.sun_family = AF_UNIX;
    strcpy(chan->remote.sun_path, uri);

    return (ubus_chan_t*)chan;
}

UBUS_STATUS ubus_status  (ubus_chan_t *s) {
    return ((ubus_channel *)s)->status;
}

int ubus_chan_fd (ubus_chan_t *s){
    ubus_channel *chan = (ubus_channel*)(s);

    return (chan->inotify)? chan->inotify : chan->fd;
}

static int do_connect(ubus_channel *chan) {
    int len;

    if ((chan->fd = socket(AF_UNIX, SOCK_STREAM, 0)) < 0) {
        chan->status = UBUS_ERROR;
        return 1;
    }

    fcntl(chan->fd, F_SETFL, fcntl(chan->fd, F_GETFL) | O_NONBLOCK);
    len = strlen(chan->remote.sun_path) + sizeof(chan->remote.sun_family);
    if (connect(chan->fd, (struct sockaddr *)&(chan->remote), len) < 0) {
        chan->status = UBUS_ERROR;
        return 1;
    }

    chan->status = UBUS_CONNECTED;
    return 0;
}

UBUS_STATUS ubus_activate (ubus_chan_t *s) {
    ubus_channel *chan = (ubus_channel *)(s);
    struct stat st;
    char *tmp, buff[2000];


    if (chan->status == UBUS_INIT) {
        //TODO: exists should stat the entire path.
        //      sometimes the stat failes because we cant read a dir
        int exists = (stat(chan->remote.sun_path,&st) > -1);
        if (exists && !do_connect(chan)) {
            if (chan->inotify)
                close(chan->inotify);
            chan->inotify = 0;
        } else if (!exists) {
            chan->inotify = inotify_init();
            fcntl(chan->inotify, F_SETFL, fcntl(chan->inotify, F_GETFL) | O_NONBLOCK);
            tmp = my_alloc(strlen(chan->remote.sun_path));
            strcpy(tmp, chan->remote.sun_path);
            if (inotify_add_watch(chan->inotify, dirname(tmp), IN_CREATE) < 0)
                chan->status = UBUS_ERROR;
            free(tmp);
            chan->status = UBUS_LURKING;
        } else {
            chan->status = UBUS_ERROR;
        }
    } else if(chan->status == UBUS_LURKING) {
        int exists = (stat(chan->remote.sun_path,&st) > -1);
        read(chan->inotify, &buff, 2000);
        if (exists) {
            if (!do_connect(chan)) {
                close(chan->inotify);
                chan->inotify = 0;
            } else {
                /*
                   FIXME
                   (file create && listen) is not atomic
                   avoid dead lock for now by waiting.
                   this should be solved with flock
                   */
                usleep(100);
                close(chan->inotify);
                chan->inotify = 0;
                do_connect(chan);
            }
        }
    } else if(chan->status == UBUS_CONNECTED)
        return UBUS_READY;

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

    if (r == 0)
        chan->status = UBUS_EOF;
    else if(r < 0)
        chan->status = UBUS_ERROR;
    else
        chan->status = UBUS_CONNECTED;

    return r;
}

void ubus_close (ubus_chan_t *s) {
    shutdown(((ubus_channel *)s)->fd, SHUT_WR);
}

void ubus_disconnect (ubus_chan_t *s){
    ubus_channel * chan = (ubus_channel*)(s);

    if (chan->service)
        ubus_client_del(chan->service, chan);
    else {
        if (chan->inotify != 0) {
            close (chan->inotify);
        }
        close(chan->fd);
        free(chan);
    }
}


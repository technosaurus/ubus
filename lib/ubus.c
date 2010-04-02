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

#include "ubus.h"

struct ubus_service_e{
    int fd;
    struct sockaddr_un local;
};

typedef struct ubus_service_e ubus_service;

void ubus_init(){
    signal (SIGPIPE,SIG_IGN);//FIXME: do in app? or at lease local and restore previous handler
}
ubus_t * ubus_create (const char * uri){
    ubus_service * service=(ubus_service*)malloc(sizeof(ubus_service));
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
int ubus_fd  (ubus_t * s){
    return ((ubus_service*)(s))->fd;
}

ubus_t * ubus_accept  (ubus_t * s){
    ubus_service * server= (ubus_service*)(s);
    struct sockaddr_un remote;
    int t = sizeof(remote);
    int client=accept(server->fd, (struct sockaddr *)&remote, &t);
    if (client==-1) {
        return 0;
    }else{
        fcntl(client, F_SETFL, fcntl(client, F_GETFL) | O_NONBLOCK);
        return (ubus_t*)client;
    }
}
void   ubus_destroy  (ubus_t * s){
    ubus_service * server= (ubus_service*)(s);
    close(server->fd);
    unlink(server->local.sun_path);
    free(server);
}
ubus_chan_t * ubus_connect  (const char * uri){
    int s;
    if ((s= socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        return 0;
    }

    struct sockaddr_un remote;
    remote.sun_family = AF_UNIX;
    strcpy(remote.sun_path, uri);
    int len = strlen(remote.sun_path) + sizeof(remote.sun_family);
    if (connect(s, (struct sockaddr *)&remote, len) == -1) {
        return 0;
    }
    return (ubus_chan_t*)s;
}
int  ubus_chan_fd   (ubus_chan_t * s){
    return (int)(s);
}
int  ubus_write  (ubus_chan_t * s, const void * buff, int len){
    return send((int)s,buff,len,0);
}
int  ubus_read   (ubus_chan_t * s, void * buff, int len){
    return recv((int)s,buff,len,0);
}
void ubus_disconnect (ubus_chan_t * s){
    shutdown((int)s, SHUT_WR);
}


#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include "ubus.h"
#include <signal.h>

int main(int argc, char ** argv){

    signal(SIGPIPE, SIG_IGN);

    if (argc < 2) {
        fprintf(stderr,"usage: echo /path/to/echo.method");
        exit(1);
    }
    ubus_t *service = ubus_create(argv[1]);
    if (service == 0) {
        perror("ubus_create");
        exit(0);
    }

    fd_set rfds;
    char buff [2048];
    for (;;) {
        FD_ZERO (&rfds);
        int maxfd = ubus_select_all(service, &rfds);
        if (select(maxfd + 2, &rfds, NULL, NULL, NULL) < 0) {
            perror("select");
            exit(1);
        }
        ubus_activate_all(service, &rfds, 0);
        ubus_chan_t * chan = 0;
        while ((chan = ubus_ready_chan(service))) {
            int len = ubus_read(chan, &buff, 2048);
            if (len < 1) {
                ubus_disconnect(chan);
            } else {
                ubus_broadcast_without(service, &buff, len, chan);
            }
        }
    }
    ubus_destroy(service);
    return 0;
}



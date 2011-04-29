#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include "ubus.h"

#define bzero(b,len) (memset((b), '\0', (len)), (void) 0)

int main(int argc, char ** argv){
    if (argc<2) {
        fprintf(stderr, "usage: clock /path/to/clock.signal\n");
        exit(1);
    }
    ubus_t *service = ubus_create(argv[1]);
    if (service == 0){
        perror("ubus_create");
        exit(0);
    }
    fd_set rfds;

    char buff [2000];
    int len = 0;;

    for (;;) {
        struct timeval tv;
        bzero(&tv, sizeof(struct timeval));
        tv.tv_sec = 60;

        FD_ZERO (&rfds);
        int maxfd = ubus_select_all(service, &rfds);
        if (select(maxfd + 2, &rfds, NULL, NULL, &tv) < 0) {
            perror("select");
            exit(1);
        }
        ubus_activate_all(service, &rfds, UBUS_IGNORE_INBOUND);

        ubus_chan_t *chan=0;
        while (chan = ubus_fresh_chan(service)) {
            ubus_write(chan, &buff, len);
        }
        FILE *f = popen("date +%H:%M","r");
        char c = fgetc(f);
        while(c != EOF){
            ubus_broadcast(service, &c, 1);
            c = fgetc(f);
        }
        pclose(f);
    }
    ubus_destroy(service);
    return 0;
}


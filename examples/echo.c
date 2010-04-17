#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include "ubus.h"

int main(int argc, char ** argv){
    if(argc<2){
        fprintf(stderr,"usage: echo /path/to/echo.method\n");
        exit(1);
    }
    ubus_t * service=ubus_create(argv[1]);
    if(service==0){
        perror("ubus_create");
        exit(0);
    }
    fd_set rfds;
    for(;;) {
        FD_ZERO (&rfds);
        int maxfd=ubus_select_all(service,&rfds);
        if (select(maxfd+2, &rfds, NULL, NULL, NULL) < 0){
            perror("select");
            exit(1);
        }
        ubus_activate_all(service,&rfds,0);
        ubus_chan_t * chan=0;
        while(chan=ubus_ready_chan(service)){
            char buff [200];
            int len=ubus_read(chan,&buff,200);
            if(len<1){
                ubus_disconnect(chan);
            }else{
                ubus_write(chan,&buff,len);
            }
        }
    }
    ubus_destroy(service);
    return 0;
}



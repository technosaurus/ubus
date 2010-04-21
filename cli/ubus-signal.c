#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include "ubus.h"
#include "tools.h"

int main(int argc, char ** argv){
    if(argc<2){
        fprintf(stderr,"usage: ubus-signal /path/to/echo.method\n");
        exit(1);
    }
    ubus_t * service=ubus_create(resolved_bus_path(argv[1]));
    if(service==0){
        perror("ubus_create");
        exit(errno);
    }
    fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);
    fd_set rfds;
    for(;;) {
        FD_ZERO (&rfds);
        FD_SET  (0, &rfds);
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
                write(1,&buff,len);
            }
        }
        if(FD_ISSET  (0, &rfds)){
            char buff [100];
            int n=read(0,&buff,100);
            if(n==0){
                ubus_destroy(service);
                exit(0);
            }
            else if(n<1){
                perror("read");
                ubus_destroy(service);
                exit(errno);
            }
            if (ubus_broadcast(service, &buff, n) < 1) {
                perror("send");
                exit(errno);
            }
        }
    }
    ubus_destroy(service);
    return 0;
}



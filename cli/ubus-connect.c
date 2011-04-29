#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include "ubus.h"
#include "tools.h"

int main(int argc, char ** argv){
    if(argc<2){
        fprintf(stderr,"usage: ubus-signal /path/to/echo.method\n");
        exit(1);
    }
    char * path=resolved_bus_path(argv[1]);
    ubus_chan_t * chan=ubus_connect(path);
    if(chan==0){
        perror("ubus_connect");
        exit(0);
    }
    if(ubus_activate(chan)==UBUS_ERROR){
        perror("ubus_activate");
        exit(0);
    }
    fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);
    fd_set rfds;
    int stdindead = 0;
    for(;;) {
        FD_ZERO (&rfds);
        if (!stdindead)
            FD_SET  (0, &rfds);
        int cfd=ubus_chan_fd(chan);
        FD_SET  (cfd, &rfds);
        if (select(cfd+2, &rfds, NULL, NULL, NULL) < 0){
            perror("select");
            exit(errno);
        }
        if(FD_ISSET(0, &rfds)){
            char buff [100];
            int n=read(0,&buff,100);
            if(n==0){
                stdindead = 1;
                ubus_close(chan);
                continue;
            }
            else if(n<1){
                perror("read");
                exit(errno);
            }
            if (ubus_write(chan, &buff, n) < 1) {
                perror("send");
                exit(errno);
            }
        }
        if(FD_ISSET(cfd, &rfds)){
            UBUS_STATUS st=ubus_activate(chan);
            if(st==UBUS_READY){
                char buff [1000];
                int n=ubus_read(chan,&buff,1000);
                if(n<1){
                    exit (0);
                }
                write(1,&buff,n);
            }else if (st==UBUS_ERROR){
                perror("ubus_activate");
                exit(errno);
            }
        }
    }
    ubus_disconnect(chan);
    return 0;
}

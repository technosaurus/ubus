#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include "common.h"

int main(int argc, char ** argv){

    //parse arguments
    int arg=1;
    const char * filename=0;
    while(argc > arg){
        if (filename==0){
            filename=argv[arg];
        }else{
            goto usage;
        }
        ++arg;
    }
    if(filename==0){
    usage:
        printf ("usage: %s /var/ipc/user/name/app/signal \n",argv[0]);
        exit (EXIT_FAILURE);
    }


    //build up server
    int server;
    struct sockaddr_un local;
    char str[100];
    if ((server = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }
    local.sun_family = AF_UNIX;
    strcpy(local.sun_path, filename);
    unlink(local.sun_path); //FIXME: really?
    if (bind(server, (struct sockaddr *)&local, strlen(local.sun_path) + sizeof(local.sun_family)) == -1) {
        perror("bind");
        exit(1);
    }
    if (listen(server, 5) == -1) {
        perror("listen");
        exit(1);
    }

    //linked list with connected clients (common.h)
    clients=NULL;

    //We'll handle that as return of read()
    signal (SIGPIPE,SIG_IGN);


    for(;;) {
        fd_set rfds;
        FD_ZERO (&rfds);
        fcntl(server, F_SETFL, fcntl(server, F_GETFL) | O_NONBLOCK);
        fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);
        FD_SET  (0, &rfds);
        FD_SET  (server, &rfds);

        if (select(server+2, &rfds, NULL, NULL, NULL) < 0){
            perror("select()");
            exit(1);
        }

        //accept new listeners
        if(FD_ISSET(server, &rfds)){
            int t;
            struct sockaddr_un remote;
            int client=accept(server, (struct sockaddr *)&remote, &t);
            if (client==-1) {
                if (errno==EAGAIN || errno==EWOULDBLOCK){
                }else{
                    perror("accept");
                    exit(1);
                }
            }else{
                fcntl(client, F_SETFL, fcntl(client, F_GETFL) | O_NONBLOCK);
                fd_add(client);
            }
        }
        //send from stdin to all listeners
        if(FD_ISSET(0, &rfds)){
            char buff [200];
            int e=read(0,&buff,200);
            if(e<0){
                perror("read");
                continue;
            }
            if(e==0){
                //FIXME: do i have to flush the other fds?
                exit(0);
            }

            client * c = clients;
            while(c){
                if (send(c->fd, &buff, e, 0) < 0) {
                    fd_del(c->fd);
                }
                c=c->next;
            }
        }
    }
    close(server);
    return 0;
}

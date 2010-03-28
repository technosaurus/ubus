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
        FD_SET  (server, &rfds);

        int maxfd=server;

        client * c=clients;
        while(c){
            FD_SET  (c->fd, &rfds);
            if(c->fd>maxfd)
                maxfd=c->fd;
            c=c->next;
        }

        if (select(maxfd+2, &rfds, NULL, NULL, NULL) < 0){
            perror("select()");
            exit(1);
        }

        //accept new callers
        if(FD_ISSET(server, &rfds)){
            struct sockaddr_un remote;
            int t = sizeof(remote);
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
        //read clients
        c=clients;
        while(c){
            char buff [1001];
            if(FD_ISSET(c->fd, &rfds)){
                int n=recv(c->fd,&buff,1000,0);
                if(n<0){
                    perror("read");
                    exit(1);
                }
                else if(n==0){
                    fd_del(c->fd);
                }else{
                    buff[n]=0;
                    printf((const char *)&buff);
                    fflush(0);

                    int i;
                    for(i=0;i<n;i++){
                        if(buff[i]=='\n'){
                            /* this is a script client.
                               we have no return value for you */
                            close(c->fd);
                            fd_del(c->fd);
                        }
                    }
                }
            }
            c=c->next;
        }
    }
    close(server);
    return 0;
}

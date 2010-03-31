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

#define MODE_TERMINAL 1
#define MODE_TAP 2
#define MODE_POOL 3
#define MODE_CONNECT 4


char mode=0;
char stdineof=0;
char ** handler=0;

void ubus_send(int s,char * msg, int size){
    if (send(s,msg ,size, 0) == -1) {
        perror("send");
    }
}

//----------------------------- list of connected clients

struct  Client_list_el{
    int fd;
    int handler;
    int handler_in;
    int handler_out;
    struct Client_list_el * next;
};

typedef struct Client_list_el Client;
Client * clients;

Client * client_add(int fd){
    Client * c=(Client *)malloc(sizeof(Client));
    c->fd=fd;
    c->handler=0;
    c->handler_in=0;
    c->handler_out=0;
    c->next=NULL;
    if(clients==NULL){
        clients=c;
        return c;
    }
    Client * cur=clients;
    while(cur){
        if(cur->next==NULL){
            cur->next=c;
            return c;
        }
        cur=cur->next;
    }
    fprintf(stderr,"corrupted linked list");
    abort();
}
void client_del(int fd){
    Client * prev=NULL;
    Client * cur=clients;
    while(cur){
        if(cur->fd==fd){
            if(prev){
                prev->next=cur->next;
            }else{
                clients=cur->next;
            }
            if(cur->handler != 0){
                close(cur->handler_in);
                close(cur->handler_out);
                kill(cur->handler,SIGTERM);
            }
            free(cur);
            return;
        }
        prev=cur;
        cur=cur->next;
    }
    fprintf(stderr,"corrupted linked list");
    abort();
}

//---------------main---------------------------------

int main(int argc, char ** argv){

    int arg=1;
    char * p_argv [argc];
    int    p_argc=0;

    const char * filename=0;

    while(argc > arg){
        if(mode==0){
            if(strcmp (argv[arg],"terminal")==0){
                mode=MODE_TERMINAL;
            }else if(strcmp (argv[arg],"tap")==0){
                mode=MODE_TAP;
            }else if(strcmp (argv[arg],"pool")==0){
                mode=MODE_POOL;
            }else if(strcmp (argv[arg],"connect")==0){
                mode=MODE_CONNECT;
            }else{
                goto usage;
            }
        } else if(argv[arg][0]=='-'  && handler==0){
            if(strcmp (argv[arg],"-h")==0){
                goto usage;
            }else{
                goto usage;
            }
        }else if (filename==0){
            filename=argv[arg];
        }else if (handler==0){
            handler=argv+arg;
        }else{
            
        }
        ++arg;
    }
    if(filename==0 ||  (mode==MODE_TERMINAL && handler==0)){
    usage:
        printf ("Usage: ubus MODE /var/ipc/user/name/app/methodname  [OPTIONS]  [handler] [handler arg1] [...]\n\n"
                "\n"
                "MODE is one of:\n"
                "\n"
                "terminal\n\n"
                "  Create a channel. \n"
                "  Spawns a new handler for each incomming channel and connects stdin/stdout.\n"
                "  The channel and the handlers are destroyed when ubus is terminated.\n"
                "  \n"
                "tap\n\n"
                "  Create a channel. \n"
                "  Write stdin to all connected channels. Read from all connected channels to stdout.\n"
                "  EOF on stdin terminates ubus. \n"
                "  The channel is destroyed when ubus is terminated.\n"
                "  \n"
                "pool\n\n"
                "  Create a channel. \n"
                "  Connect all stdings to all stdouts.\n"
                "  EOF on stdin terminates ubus. \n"
                "  The channel is destroyed when ubus is terminated.\n"
                "  \n"
                "connect\n\n"
                "  connect stdin/stdout to an existing channel.\n"
                "  EOF on stdin terminates ubus. \n"
                "  \n"
                );
        exit (EXIT_FAILURE);
    }

    if(mode==MODE_TERMINAL || mode==MODE_TAP  || mode==MODE_POOL){
        //---------------server mode---------------------------------
        int server;
        struct sockaddr_un local;
        if ((server = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
            perror("socket");
            exit(1);
        }
        local.sun_family = AF_UNIX;
        strcpy(local.sun_path, filename);
        unlink(local.sun_path); //FIXME: find better way
        if (bind(server, (struct sockaddr *)&local, strlen(local.sun_path) + sizeof(local.sun_family)) == -1) {
            perror("bind");
            exit(1);
        }
        if (listen(server, 5) == -1) {
            perror("listen");
            exit(1);
        }

        //linked list with connected clients
        clients=NULL;

        //We'll handle that as return of read()
        signal (SIGPIPE,SIG_IGN);

        for(;;) {
            fd_set rfds;
            FD_ZERO (&rfds);

            if(mode!=MODE_TERMINAL){
                fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);
                FD_SET  (0, &rfds);
            }

            fcntl(server, F_SETFL, fcntl(server, F_GETFL) | O_NONBLOCK);
            FD_SET  (server, &rfds);

            int maxfd=server;
            Client * c=clients;
            while(c){
                FD_SET  (c->fd, &rfds);
                if(c->fd>maxfd){
                    maxfd=c->fd;
                }
                if(mode==MODE_TERMINAL){
                    FD_SET  (c->handler_out, &rfds);
                    if(c->handler_out>maxfd){
                        maxfd=c->handler_out;
                    }
                }

                c=c->next;
            }
            if (select(maxfd+2, &rfds, NULL, NULL, NULL) < 0){
                perror("select");
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
                    Client *cl = client_add(client);
                    if(mode==MODE_TERMINAL){
                        int pipe_in[2];
                        int pipe_out[2];
                        if (pipe(pipe_in) < 0){
                            perror ("pipe");
                            exit (errno);
                        }
                        if (pipe(pipe_out) < 0){
                            perror ("pipe");
                            exit (errno);
                        }

                        cl->handler=fork();
                        if(cl->handler<0){
                            perror("fork");
                            exit(1);
                        }
                        if(cl->handler==0){
                            //i'm the child. wee

                            close (pipe_in[1]);
                            dup2 (pipe_in[0], 0);
                            close (pipe_in[0]);

                            close (pipe_out[0]);
                            dup2 (pipe_out[1], 1);
                            close (pipe_out[1]);
                            execvp (handler[0],handler);
                            perror("execvp");
                            exit(1);
                        }
                        cl->handler_in=pipe_in[1];
                        cl->handler_out=pipe_out[0];
                    }
                }
            }

            //read clients
            c=clients;
            while(c){
                if(FD_ISSET(c->fd, &rfds)){
                    char buff [1000];
                    int n=recv(c->fd,&buff,1000,0);
                    if (n<1){
                        int ff=c->fd;
                        c=c->next;
                        client_del(ff);
                        continue;
                    }

                    if(mode==MODE_TERMINAL){
                        //FIXME: this may block :(
                        write(c->handler_in,&buff,n);
                    }else if (mode==MODE_TAP){
                        write(1,&buff,n);
                    }else if (mode==MODE_POOL){
                        write(1,&buff,n);
                        Client * c2=clients;
                        while(c2){
                            if (send(c2->fd,&buff ,n, 0) <0) {
                                perror("send");
                                if(c!=c2){
                                    int ff=c2->fd;
                                    c2=c2->next;
                                    client_del(ff);
                                    continue;
                                }
                            }
                            c2=c2->next;
                        }
                    }
                }
                if(mode==MODE_TERMINAL){
                    if(FD_ISSET(c->handler_out, &rfds)){
                        char buff [1000];
                        int n=read(c->handler_out,&buff,1000);
                        if (n<1){
                            if(n<0){
                                perror("handler");
                            }
                            int ff=c->fd;
                            c=c->next;
                            client_del(ff);
                            continue;
                        }
                        send(c->fd,&buff,n,0);
                    }
                }
                c=c->next;
            }

            if(mode!=MODE_TERMINAL){
                if(FD_ISSET(0, &rfds)){
                    char buff [1000];
                    int e=read(0,&buff,1000);
                    if(e<0){
                        perror("read");
                        continue;
                    }
                    if(e==0){
                        //FIXME: do i have to flush the other fds?
                        exit(0);
                    }
                    Client * c = clients;
                    while(c){
                        if (send(c->fd, &buff, e, 0) < 0) {
                            client_del(c->fd);
                        }
                        c=c->next;
                    }
                    if(mode==MODE_POOL){
                        write(1,&buff,e);
                    }
                }
            }

        }
        close(server);
    }else{
        //---------------client mode---------------------------------
        int s;
        if ((s= socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
            perror("socket");
            exit(1);
        }

        struct sockaddr_un remote;
        remote.sun_family = AF_UNIX;
        strcpy(remote.sun_path, filename);
        int len = strlen(remote.sun_path) + sizeof(remote.sun_family);
        if (connect(s, (struct sockaddr *)&remote, len) == -1) {
            perror("connect");
            exit(1);
        }



        for(;;) {
            fd_set rfds;
            FD_ZERO (&rfds);
            fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);
            FD_SET  (0, &rfds);
            fcntl(s, F_SETFL, fcntl(s, F_GETFL) | O_NONBLOCK);
            FD_SET  (s, &rfds);

            if (select(s+2, &rfds, NULL, NULL, NULL) < 0){
                perror("select");
                exit(1);
            }


            if(FD_ISSET(s, &rfds)){
                char buff [1000];
                int n=recv(s,&buff,1000,0);
                if(n<1){
                    exit (0);
                }
                write(1,&buff,n);
            }

            if(FD_ISSET(0, &rfds)){
                char buff [1000];
                int n=read(0,&buff,1000);
                if(n==0){
                    exit(0);
                }
                else if(n<1){
                    perror("read");
                    exit(errno);
                }
                if (send(s, &buff, n, 0) < 0) {
                    perror("send");
                    exit(1);
                }
            }
        }
        close(s);
    }
    return 0;
}



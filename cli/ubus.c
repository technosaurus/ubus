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

#define MODE_TERMINAL 1
#define MODE_TAP 2
#define MODE_POOL 3
#define MODE_CONNECT 4

char stdineof=0;
char ** handler=0;
char mode=0;

int si_handler=0;
int si_handler_in=0;
int si_handler_out=0;

struct  ubus_client_list_el{
    ubus_chan_t * chan;
    int handler;
    int handler_in;
    int handler_out;
    struct ubus_client_list_el * next;
    struct ubus_client_list_el * prev;
};
typedef struct ubus_client_list_el ubus_client;
ubus_client * clients;

static ubus_client * ubus_client_add(ubus_chan_t * chan){
    ubus_client * c=(ubus_client *)malloc(sizeof(ubus_client));
    c->chan=chan;
    c->handler=0;
    c->handler_in=0;
    c->handler_out=0;
    c->next=NULL;
    c->prev=NULL;
    if(clients==NULL){
        clients=c;
        return c;
    }
    ubus_client * cur=clients;
    while(cur){
        if(cur->next==NULL){
            cur->next=c;
            c->prev=cur;
            return c;
        }
        cur=cur->next;
    }
    fprintf(stderr,"corrupted linked list");
    abort();
}
static ubus_client *  ubus_client_del(ubus_client * c){
    ubus_client * n= NULL;
    if(c->prev==NULL){
        if(c!=clients){
            fprintf(stderr,"corrupted linked list");
            abort;
        }
        clients=0;
    }else{
        ubus_client * n= c->next;
        c->prev->next=c->next;
    }
    if(c->handler != 0){
        close(c->handler_in);
        close(c->handler_out);
        kill(c->handler,SIGTERM);
    }
    ubus_disconnect(c->chan);
    free(c);
    return n;
}

static int ubus_fork_handler(char ** args,int * in,int * out){
    int r=0;
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

    r=fork();
    if(r<0){
        perror("fork");
        exit(1);
    }
    if(r==0){
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
    close (pipe_in[0]);
    *in=pipe_in[1];

    close (pipe_out[1]);
    *out=pipe_out[0];

    return r;
}

static void ubus_broadcast(const void * buff,int size) {
    ubus_client * c2=clients;
    while(c2){
        if (ubus_write(c2->chan,buff,size)<1){
            perror("send");
            c2=ubus_client_del(c2);
            continue;
        }
        c2=c2->next;
    }
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
    if(filename==0 || (mode==MODE_TERMINAL && handler==0)){
    usage:
        fprintf (stderr,"Usage: ubus MODE /var/ipc/user/name/app/methodname  [OPTIONS] [handler] [handler arg1] [...]\n\n"
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
                "  If a handler is given, spawn it (once!) instead of stdin/out \n"
                "  The channel and the handler are destroyed when ubus is terminated.\n"
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

    if(mode!=MODE_CONNECT){
        //---------------server mode---------------------------------
        ubus_t * s=ubus_create(filename);
        if(s==0){
            perror("ubus_create");
            exit(0);
        }

        if (mode==MODE_TAP && handler!=0){
            si_handler=ubus_fork_handler(handler,&si_handler_in,&si_handler_out);
        }

        //linked list with connected clients
        clients=NULL;

        fd_set rfds;
        for(;;) {
            FD_ZERO (&rfds);

            //select service
            int server=ubus_fd(s);
            FD_SET  (server, &rfds);
            int maxfd=server;

            //select stdin
            if(handler==0){
                fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);
                FD_SET  (0, &rfds);
            }

            //select si_handler
            if(si_handler!=0){
                FD_SET  (si_handler_out, &rfds);
                if(si_handler_out>maxfd) maxfd=si_handler_out;
            }

            //select clients
            ubus_client* c=clients;
            while(c){
                int fd=ubus_chan_fd(c->chan);
                FD_SET  (fd, &rfds);
                if(fd>maxfd) maxfd=fd;
                //select on client handler
                if(c->handler!=0){
                    FD_SET  (c->handler_out, &rfds);
                    if(c->handler_out>maxfd) maxfd=c->handler_out;
                }
                c=c->next;
            }

            //run select
            if (select(maxfd+2, &rfds, NULL, NULL, NULL) < 0){
                perror("select");
                exit(1);
            }

            //accept new callers
            if(FD_ISSET(server, &rfds)){
                ubus_chan_t * client =ubus_accept (s);
                if (client==0) {
                    if (errno==EAGAIN || errno==EWOULDBLOCK){
                    }else{
                        perror("ubus_accept");
                        exit(1);
                    }
                }else{
                    ubus_client * cl = ubus_client_add(client);
                    if(mode==MODE_TERMINAL){
                        cl->handler=ubus_fork_handler(handler,&(cl->handler_in),&(cl->handler_out));
                    }
                }
            }

            //read si_handler
            if(si_handler!=0){
                if(FD_ISSET(si_handler_out, &rfds)){
                    char buff [100];
                    int n=read(si_handler_out,&buff,100);
                    ubus_broadcast(&buff,n);
                }
            }

            //read clients
            c=clients;
            while(c){
                if(c->handler!=0){
                    //read client handler
                    if(FD_ISSET(c->handler_out, &rfds)){
                        char buff [100];
                        int n=read(c->handler_out,&buff,100);
                        if (n<1){
                            if(n<0){
                                perror("handler");
                            }
                            c=ubus_client_del(c);
                            continue;
                        }
                        ubus_write(c->chan,&buff,n);
                    }
                }
                int fd=ubus_chan_fd(c->chan);
                if(FD_ISSET(fd, &rfds)){
                    char buff [100];
                    int n=ubus_read(c->chan,&buff,100);
                    if(n==0 && c->handler!=0 ){
                        close(c->handler_in);
                        c=c->next;
                        continue;
                    }
                    else if (n<1){
                        c=ubus_client_del(c);
                        continue;
                    }

                    if(si_handler!=0){
                        write(si_handler_in,&buff,n);
                    } else if (c->handler!=0) {
                        write(c->handler_in,&buff,n);
                    }else{
                        write(1,&buff,n);
                        if (mode==MODE_POOL){
                            c=c->next; //could get deleted
                            ubus_broadcast(&buff,n);
                            continue;
                        }
                    }
                }
                c=c->next;
            }

            //read stdin
            if(handler==0){
                if(FD_ISSET(0, &rfds)){
                    char buff [1000];
                    int n=read(fileno(stdin),&buff,1000);
                    if(n<0){
                        perror("read");
                        exit(errno);
                    } else if(n==0){
                        //FIXME: do i have to flush the other fds?
                        exit(0);
                    }
                    ubus_broadcast(&buff,n);
                    if(mode==MODE_POOL){
                        write(fileno(stdout),&buff,n);
                    }
                }
            }

        }//end main loop
        ubus_destroy(s);
    }else{
        //---------------client mode---------------------------------
        ubus_chan_t * chan=ubus_connect(filename);
        if(chan==0){
            perror("ubus_connect");
            exit(errno);
        }

        for(;;) {
            fd_set rfds;
            FD_ZERO (&rfds);
            fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);
            FD_SET  (0, &rfds);
            int s=ubus_chan_fd(chan);
            fcntl(s, F_SETFL, fcntl(s, F_GETFL) | O_NONBLOCK);
            FD_SET  (s, &rfds);

            if (select(s+2, &rfds, NULL, NULL, NULL) < 0){
                perror("select");
                exit(1);
            }

            if(FD_ISSET(s, &rfds)){
                char buff [1000];
                int n=ubus_read(chan,&buff,1000);
                if(n<1){
                    exit (0);
                }
                write(1,&buff,n);
            }

            if(FD_ISSET(0, &rfds)){
                char buff [100];
                int n=read(0,&buff,100);
                if(n==0){
                    ubus_disconnect(chan);
                    continue;
                }
                else if(n<1){
                    perror("read");
                    exit(errno);
                }
                if (ubus_write(chan, &buff, n) < 1) {
                    perror("send");
                    exit(1);
                }
            }
        }
    }
    return 0;
}



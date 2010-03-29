#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <signal.h>

#define MODE_MET 1
#define MODE_CAL 2
#define MODE_SIG 3
#define MODE_LIS 4


char mode=0;
char once=0;
char readresp=0;
char rawprint=0;
char rawread=0;
char inarg=0;
char noreturn=0;
char noquote=0;
char argterm=0;

char stdineof=0;


void ubus_send(int s,char * msg, int size){
    if (send(s,msg ,size, 0) == -1) {
        perror("send");
        if(mode==MODE_CAL || mode==MODE_LIS)
            exit(1);
    }
}

void print_response_buffer(char * buff,int bufflen){
    if(rawprint){
        write(1,buff,bufflen);
        write(1,"\n",1);
    }else{
        int offset=0;
        char * ob= (char*)malloc(bufflen);
        while(offset<bufflen){
            if(noquote==0)
                putchar('"');
            int s=ubus_next_arg(buff, ob,bufflen,&offset);
            int i=0;
            for(;i<s;i++){
                if(noquote==0){
                    if(ob[i]=='"'){
                        putchar('\\');
                    }
                }
                putchar(ob[i]);
            }
            if(noquote==0)
                putchar('"');
            putchar('\t');
            putchar(' ');
        }
        putchar('\n');
    }
    fflush(0);
}


char ubus_read_response(int in,char ** cbuff, int * cbufflen){
    int hits=0;
    char buff [1001];
    int n=recv(in,&buff,1000,0);
    if(n<0){
        perror("read");
        return -1;
    }
    else if(n==0){
        return -2;
    }else{
        buff[n]=0;
        if(cbuff==0){
            (*cbuff)=malloc(n);
        }else{
            //FIXME: do we care about buffer bloat?
            (*cbuff)=realloc((*cbuff),(*cbufflen)+n);
        }
        int i=0;
        for(;i<n;i++){
            if(buff[i]=='\n'){
                print_response_buffer((*cbuff),(*cbufflen));
                free((*cbuff));
                (*cbuff)=0;
                (*cbufflen)=0;
                if((n-i-1)>0){
                    (*cbuff)=malloc(n-i-1);
                }
                fflush(0);
                hits++;
                continue;
            }
            (*cbuff)[(*cbufflen)++]=buff[i];
        }
    }
    return hits;
}

int ubus_read_call( int out){
    char didsomethingyet=0;
    char c;
    for(;;){
        int r=read(0,&c,1);
        if(r<1){
            if(didsomethingyet==0){
                return -1;
            }
            stdineof=1;
            if(inarg==1){
                ubus_send(out,"\n",1);
            }
            return 0 ;
        }
        didsomethingyet=1;
        if(inarg==1){
            if (c=='\n'){
                ubus_send(out,"\\n", 2);
            }else if (c=='\t'){
                ubus_send(out,"\\t", 2);
            }else if (c=='\\'){
                ubus_send(out,"\\\\", 2);
            }else{
                ubus_send(out,&c,1);
            }
        }else{
            ubus_send(out,&c,1);
        }
        if( c=='\n' && (rawread==1 || (mode==MODE_MET && readresp==1))){
            return 0;
        }
    }
}

char * ubus_escaped_arg(const char * a, int len, int * outlen){
    int alloced=len+2;
    char * r=(char*)malloc(alloced+1);
    (*outlen)=0;
    int more=0;
    int k;
    for(k=0;k<len;k++){
        char c=a[k];
        if(c=='\\' || c=='\n' || c=='\t'){
            if((more+len)==alloced){
                alloced+=32;
                r=realloc ( r, alloced+1);
            }
            r[(*outlen)++]='\\';
            if(c=='\n'){
                r[(*outlen)++]='n';
            }else if(c=='\t'){
                r[(*outlen)++]='t';
            }else{
                r[(*outlen)++]=c;
            }
            more++;
        }else{
            r[(*outlen)++]=c;
        }
    }
    return r;
}

int ubus_next_arg(const char * in,  char * out, int size, int * offset){

    char esc=0;
    int rp=0;
    while(*offset < size){
        char c=in[(*offset)++];
        if (c=='\n' || c=='\t'){
            return rp;
        } else if(esc==1){
            if(c=='\\'){
                out[rp++]='\\';
            }else  if(c=='n'){
                out[rp++]='\n';
            }else  if(c=='t'){
                out[rp++]='\t';
            }
            esc=0;
        }else if(c=='\\'){
            esc=1;
        }else{
            out[rp++]=c;
        }
    }
    return rp;
}


//----------------------------- list of connected clients

struct  client_list_el{
    char * buff;
    int bufflen;
    int fd;
    struct client_list_el * next;
};

typedef struct client_list_el client;
client * clients;

void client_add(int fd){
    client * c=(client *)malloc(sizeof(client));
    c->fd=fd;
    c->buff=0;
    c->bufflen=0;
    c->next=NULL;
    if(clients==NULL){
        clients=c;
        return;
    }
    client * cur=clients;
    while(cur){
        if(cur->next==NULL){
            cur->next=c;
            return;
        }
        cur=cur->next;
    }
    fprintf(stderr,"corrupted linked list");
    abort();
}
void client_del(int fd){
    client * prev=NULL;
    client * cur=clients;
    while(cur){
        if(cur->fd==fd){
            if(prev){
                prev->next=cur->next;
            }else{
                clients=cur->next;
            }
            if(cur->buff)
                free(cur->buff);
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
            if(strcmp (argv[arg],"method")==0){
                mode=MODE_MET;
            }else if(strcmp (argv[arg],"call")==0){
                mode=MODE_CAL;
            }else if(strcmp (argv[arg],"signal")==0){
                mode=MODE_SIG;
            }else if(strcmp (argv[arg],"listen")==0){
                mode=MODE_LIS;
            }else{
                goto usage;
            }
        } else if(argv[arg][0]=='-' &&  argterm==0){
            if(strcmp (argv[arg],"-s")==0 &&  rawread==0){
                inarg=1;
            }else if(strcmp (argv[arg],"--")==0){
                argterm=1;
            }else if(strcmp (argv[arg],"-r")==0 && inarg==0){
                rawread=1;
            }else if(strcmp (argv[arg],"-w")==0){
                rawprint=1;
            }else if(strcmp (argv[arg],"-1")==0){
                once=1;
            }else if(strcmp (argv[arg],"-n")==0){
                noreturn=1;
            }else if(strcmp (argv[arg],"-m")==0){
                noquote=1;
            }else if(strcmp (argv[arg],"-d")==0){
                readresp=1;
            }else if(strcmp (argv[arg],"-h")==0){
                goto usage;
            }else{
                goto usage;
            }
        }else if (filename==0){
            filename=argv[arg];
        }else if (inarg==0 && rawread==0){
            p_argv[p_argc++]=argv[arg];
        }
        ++arg;
    }
    if(filename==0){
    usage:
        printf ("Usage: ubus MODE /var/ipc/user/name/app/methodname [OPTIONS] [--] [arguments..]\n\n"
                "the -s,-r and [arguments..]  are mutally exclusive \n\n"
                "-- : terminate options. only call arguments follow.\n"
                "-s : read stdin until EOF as the one and only argument.\n"
                "-r : read raw wire data from stdin. Terminate with newline.\n"
                "-w : print raw wire format.\n"
                "-m : do not quote arguments in output.\n"
                "\n"
                "MODE is one of:\n"
                "\n"
                "method\n\n"
                "  Create a method and wait for calls. Arguments will be printed to \n"
                "  stdout, quoted, seperated by space, and terminated with newline.  \n"
                "  Unless -d is specified, it will auto-respond with an empty message. \n"
                "  The method is destroyed when ubus ends.\n"
                "  \n"
                "  -d : read response from stdin. blocks all other clients.\n"
                "  -1 : quit after sending the response. \n"
                "  \n"
                "call\n\n"
                "  Calls a method with [arguments..] and prints the return.\n"
                "  \n"
                "  -1 : quit after receiving the first response.\n"
                "  -n : do not read the return value.\n"
                "  \n"
                "signal\n\n"
                "  Create a signal. read a single argument from stdin terminated by newline. \n"
                "  Each newline fires the signal again. EOF quits ubus.\n"
                "  The signal is destroyed when ubus ends.\n"
                "  \n"
                "listen\n\n"
                "  Waits for a signal to fire. Arguments will be printed to \n"
                "  stdout, quoted, seperated by space, and terminated with newline.  \n"
                "  \n"
                "  -1 : quit after first signal received.\n"
                "  \n"
                );
        exit (EXIT_FAILURE);
    }

    if(mode==MODE_MET || mode==MODE_SIG){
        //---------------server mode---------------------------------
        int server;
        struct sockaddr_un local;
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

        //linked list with connected clients
        clients=NULL;

        //We'll handle that as return of read()
        signal (SIGPIPE,SIG_IGN);


        for(;;) {
            fd_set rfds;
            FD_ZERO (&rfds);
            fcntl(server, F_SETFL, fcntl(server, F_GETFL) | O_NONBLOCK);
            FD_SET  (server, &rfds);
            if(mode==MODE_SIG){
                fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);
                FD_SET  (0, &rfds);
            }
            int maxfd=server;
            if(mode==MODE_MET){
                client * c=clients;
                while(c){
                    FD_SET  (c->fd, &rfds);
                    if(c->fd>maxfd)
                        maxfd=c->fd;
                    c=c->next;
                }
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
                    client_add(client);
                }
            }

            if(mode==MODE_MET){
                //read clients
                client*c=clients;
                while(c){
                    if(FD_ISSET(c->fd, &rfds)){
                        int r= ubus_read_response(c->fd,&c->buff,&c->bufflen);
                        if (r<0){
                            client_del(c->fd);
                        }else{
                            while((r--)>0){
                                if(readresp==1){
                                    ubus_read_call(c->fd);
                                }else{
                                    ubus_send(c->fd,"\n",1);
                                }
                            }
                        }
                    }
                    c=c->next;
                }
            }else if(mode==MODE_SIG){
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
                            client_del(c->fd);
                        }
                        c=c->next;
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

        for(;;){
            //call mode
            if(mode==MODE_CAL){
                if(inarg==1 || rawread==1){
                    if(ubus_read_call(s)<0){
                        exit(0);
                    }
                }else{
                    int i;
                    for(i=0;i<p_argc;i++){
                        int ln;
                        char * x = (char*)ubus_escaped_arg(p_argv[i],strlen(p_argv[i]),&ln);
                        ubus_send(s,x,ln);
                        free(x);
                        ubus_send(s,"\t",1);
                    }
                    ubus_send(s,"\n",1);
                }
            } //end call mode

            if(noreturn==0){
                char *buff=0;;
                int bufflen=0;
                int r= ubus_read_response(s,&buff,&bufflen);
            }

            if(once || ( mode==MODE_CAL && inarg==0 && rawread==0 ) || (stdineof==1) ){
                break;
            }
        }
        close(s);
    }
    return 0;
}



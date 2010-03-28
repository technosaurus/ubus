#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>

int main(int argc, char ** argv){

    int arg=1;
    char pipearg=0;
    char argterm=0;

    char * p_argv [argc];
    int    p_argc=0;

    const char * filename=0;

    while(argc > arg){
        if(argv[arg][0]=='-' &&  argterm==0){
            if(strcmp (argv[arg],"-i")==0){
                pipearg=1;
            }else if(strcmp (argv[arg],"--")==0){
                argterm=1;
            }else if(strcmp (argv[arg],"-h")==0){
                goto usage;
            }else{
                goto usage;
            }
        }else if (filename==0){
            filename=argv[arg];
        }else if (pipearg==0){
            p_argv[p_argc++]=argv[arg];
        }
        ++arg;
    }
    if(filename==0){
    usage:
        printf ("Usage: ubus-call /var/ipc/user/name/app/method [OPTIONS] [--] [arguments..]\n\t\n"
                "-h  : usage\n"
                "-i  : read single argument from stdin.\n"
                "-r  : read raw call\n"
                "--  : everything following is an argument to the call\n"
                );
        exit (EXIT_FAILURE);
    }

    int s, t, len;
    struct sockaddr_un remote;
    char str[100];

    if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    remote.sun_family = AF_UNIX;
    strcpy(remote.sun_path, filename);
    len = strlen(remote.sun_path) + sizeof(remote.sun_family);
    if (connect(s, (struct sockaddr *)&remote, len) == -1) {
        perror("connect");
        exit(1);
    }

    if(pipearg){
        char c;
        for(;;){
            c=getchar();
            if (send(s, &c, 1, 0) == -1) {
                perror("send");
                exit(1);
            }
            if(c==EOF || c=='\n'){
                break;
            }
        }
    }else{
        int i;
        for(i=0;i<p_argc;i++){
            int ln;
            char * x = (char*)ubus_escaped_arg(p_argv[i],strlen(p_argv[i]),&ln);
            if (send(s,x,ln, 0) == -1) {
                perror("send");
                exit(1);
            }
            free(x);
        }
    }
    //
    if (shutdown(s,SHUT_WR)==-1) {
        perror("shutdown");
        exit(1);
    }

    for(;;){
        if ((t=recv(s, str, 100, 0)) > 0) {
            str[t] = '\0';
            printf(str);
            fflush(0);
            if(t && str[t-1]=='\n'){
                break;
            }
        } else {
            if (t < 0)
                perror("recv");
            exit(1);
        }
    }
    close(s);
    return 0;
}



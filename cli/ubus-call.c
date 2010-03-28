#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>

int main(int argc, char ** argv){

    int arg=1;
    char once=0;
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
        printf ("usage: %s /var/ipc/user/name/app/method \n",argv[0]);
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

    for(;;){
        if ((t=recv(s, str, 100, 0)) > 0) {
            str[t] = '\0';
            printf(str);
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



#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <libgen.h>


int mkpath(const char *s, mode_t mode){
    char *q, *r = NULL, *path = NULL, *up = NULL;
    int rv;

    rv = -1;
    if (strcmp(s, ".") == 0 || strcmp(s, "/") == 0)
        return (0);

    if ((path = strdup(s)) == NULL)
        exit(1);

    if ((q = strdup(s)) == NULL)
        exit(1);

    if ((r = dirname(q)) == NULL)
        goto out;

    if ((up = strdup(r)) == NULL)
        exit(1);

    if ((mkpath(up, mode) == -1) && (errno != EEXIST))
        goto out;

    if ((mkdir(path, mode) == -1) && (errno != EEXIST))
        rv = -1;
    else
        rv = 0;

 out:
    if (up != NULL)
        free(up);
    free(q);
    free(path);
    return (rv);
}

int mksocket(const char *s){
    mkpath(dirname((char*)s),S_IRWXU|S_IRWXG);
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

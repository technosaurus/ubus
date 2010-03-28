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



char * ubus_escaped_arg(const char * a, int len, int * outlen){
    int alloced=len+2;
    char * r=(char*)malloc(alloced+1);
    (*outlen)=0;
    int more=0;
    int k;
    for(k=0;k<len;k++){
        char c=a[k];
        if(c=='\\' || c=='\n'){
            if((more+len)==alloced){
                alloced+=32;
                r=realloc ( r, alloced+1);
            }
            r[(*outlen)++]='\\';
            if(c=='\n'){
                r[(*outlen)++]='n';
            }else{
                r[(*outlen)++]=c;
            }
            more++;
        }else{
            r[(*outlen)++]=c;
        }
    }
    r[(*outlen)++]='\n';
    r[(*outlen)]=0;
    return r;
}

int ubus_next_arg(const char * in,  char * out, int size, int * offset){

    char esc=0;
    int rp=0;
    while(*offset < size){
        char c=in[(*offset)++];
        if (c=='\n'){
            return rp;
        } else if(esc==1){
            if(c=='\\'){
                out[rp++]='\\';
            }else  if(c=='n'){
                out[rp++]='\n';
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

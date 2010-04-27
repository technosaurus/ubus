#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include "ubus.h"
#include "tools.h"


#define BITCH_ABOUT_NULLPTR(x) if (x==NULL){fprintf(stdout,"\a666\tcan't parse that.\n");fflush(stdout); continue;}

struct  ubus_list_el{
    ubus_t * bus;
    char * ident;
    struct ubus_list_el * next;
    struct ubus_list_el * prev;
};
static struct ubus_list_el * buslist=NULL;

static struct ubus_list_el * l_add(ubus_t * bus){
    struct ubus_list_el* c=(struct ubus_list_el *)malloc(sizeof(struct ubus_list_el));
    c->bus=bus;
    c->next=NULL;
    c->prev=NULL;
    c->ident=NULL;
    if(buslist==NULL){
        buslist=c;
        return c;
    }
    struct ubus_list_el * cur=buslist;
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
    return 0;
}
static struct ubus_list_el * l_del(struct ubus_list_el * c){
    struct ubus_list_el * n= NULL;
    if(c->prev==NULL){
        if(c!=buslist){
            fprintf(stderr,"corrupted linked list");
            abort;
        }
        buslist=0;
    }else{
        struct ubus_list_el* n= c->next;
        c->prev->next=c->next;
    }
    ubus_destroy(c->bus);
    free(c->ident);
    free(c);
    return n;
}


static struct ubus_list_el * find_by_ident(char * ident){
    struct ubus_list_el * cur=buslist;
    char found=0;
    while(cur){
        if(strcmp(cur->ident,ident)==0){
            return cur;
            break;
        }
        cur=cur->next;
    }
    return 0;
}

static void cleanup(){
    while(buslist){
        fprintf(stderr,"%p",buslist);
        buslist=l_del(buslist);
    }
}

int main(int argc, char ** argv){
    atexit(cleanup);

    fd_set rfds;
    fcntl(0, F_SETFL, fcntl(0, F_GETFL) | O_NONBLOCK);
    for(;;) {
        FD_ZERO (&rfds);
        FD_SET  (0, &rfds);
        int maxfd=0;
        struct ubus_list_el * cur=buslist;
        while(cur){
            int m=ubus_select_all(cur->bus,&rfds);
            if(m>maxfd)
                maxfd=m;
            cur=cur->next;
        }

        if (select(maxfd+2, &rfds, NULL, NULL, NULL) < 0){
            perror("select");
            exit(1);
        }

        cur=buslist;
        while(cur){
            ubus_activate_all(cur->bus,&rfds,0);
            ubus_chan_t * c;
            while(c=ubus_fresh_chan (cur->bus)){
                printf("c\t%s\t%i\n",(int)cur->ident,(int)c);
                fflush(stdout);
            }
            while(c=ubus_ready_chan (cur->bus)){
                static char buff [256002];
                int len=ubus_read(c,&buff,256001);
                buff[len]=0;
                if(len>=256001){
                    static const char * overflowe="\a75\tcall too large.\n";
                    ubus_write(c,overflowe,strlen(overflowe));
                } else if(len>0){
                    printf("r\t%s\t%i\t%s",(int)cur->ident,(int)c,&buff);
                    fflush(stdout);
                }else{
                    printf("d\t%s\t%i\n",(int)cur->ident,(int)c);
                    fflush(stdout);
                }
            }
            cur=cur->next;
        }

        if(FD_ISSET(0, &rfds)){
            static char buff [256002];
            int n=read(0,&buff,256001);
            buff[n]=0;
            if(n>=256001){
                fprintf(stdout,"\a75\tcall too large.\n");
                continue;
            }else if(n<0){
                perror("read");
                exit(errno);
            } else if(n==0){
                exit(0);
            }

            char * saveptr;
            char * op=strtok_r((char*)&buff, "\t\n", &saveptr);
            BITCH_ABOUT_NULLPTR(op);

            switch (op[0]){
                case 'b':{
                    char * ident=strtok_r(0, "\t\n", &saveptr);
                    BITCH_ABOUT_NULLPTR(ident);
                    char * path=strtok_r(0, "\t\n", &saveptr);
                    BITCH_ABOUT_NULLPTR(path);

                    ubus_t * bus=ubus_create(resolved_bus_path(path));
                    if(bus){
                        fprintf(stdout,"b\t%s\t1\n",ident);
                        struct ubus_list_el *el=  l_add(bus);
                        el->ident=malloc(strlen(ident)+1);
                        strcpy(el->ident,ident);
                    }else{
                        fprintf(stdout,"b\t%s\t0\t%i\tshit broke\n",ident,errno);
                        fflush(stdout);
                    }

                    break;;
                }
                case 'u':{
                    char * ident=strtok_r(0, "\t\n", &saveptr);
                    BITCH_ABOUT_NULLPTR(ident);
                    struct ubus_list_el * cur=find_by_ident(ident);
                    if(cur==0){
                        fprintf(stdout,"\a667\tno such channel.\n"); continue;
                        continue;
                    }else{
                        l_del(cur);
                    }
                    break;;
                }
                case 'w':{
                    char * ident=strtok_r(0, "\t\n", &saveptr);
                    BITCH_ABOUT_NULLPTR(ident);
                    char * chan_s=strtok_r(0, "\t\n", &saveptr);
                    BITCH_ABOUT_NULLPTR(chan_s);

                    struct ubus_list_el * cur=find_by_ident(ident);
                    if(cur==0){
                        fprintf(stdout,"\a667\tno such channel.\n"); continue;
                        continue;
                    }else{
                        char * rest=strtok_r(0, "", &saveptr);;
                        if(chan_s[0]=='*'){
                            ubus_broadcast(cur->bus,rest,strlen(rest));
                        }else{
                            ubus_chan_t * chan= (ubus_chan_t*)(atoi(chan_s));
                            ubus_write(chan,rest,strlen(rest));
                        }
                    }
                    break;
                }
                case 'd':{
                    char * ident=strtok_r(0, "\t\n", &saveptr);
                    BITCH_ABOUT_NULLPTR(ident);
                    char * chan_s=strtok_r(0, "\t\n", &saveptr);
                    BITCH_ABOUT_NULLPTR(chan_s);

                    struct ubus_list_el * cur=find_by_ident(ident);
                    if(cur==0){
                        fprintf(stdout,"\a667\tno such channel.\n"); continue;
                        continue;
                    }else{
                        char * rest=strtok_r(0, "", &saveptr);;
                        if(chan_s[0]=='*'){
                            fprintf(stdout,"\a668\tcant disconnect all (yet).\n"); continue;
                        }else{
                            ubus_chan_t * chan= (ubus_chan_t*)(atoi(chan_s));
                            ubus_disconnect(chan);
                        }
                    }
                    break;
                }
                default:
                    fprintf(stdout,"\a669\twhat?\n");
                    fflush(stdout);
            }
        }
    }
    return 0;
}



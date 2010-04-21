#include <string.h>
static char * resolved_bus_path(char * arg){
    if (arg[0]!='.' && arg[0]!='/' && arg[0]!=0){
        const char * home=getenv ("HOME");
        const char * s = "/.ipc/";

        static char * path=0;
        if(path!=0){
            free(path);
        }
        path= malloc(sizeof(char)*(strlen(home)+strlen(s)+strlen(arg)+2));
        strcpy(path,home);
        strcat(path,s);
        strcat(path,arg);
        return path;
    }
    return arg;
}



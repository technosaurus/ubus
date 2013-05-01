#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include "ubus.h"
#include "tools.h"
#include <signal.h>


void timeout(int sig) {
    fprintf(stderr, "timed out\n");
    exit (142);
}

int main(int argc, char ** argv) {

    signal(SIGALRM, timeout);

    int ok = 1;
    long int waitAnswers = 0;
    long int waitSeconds = 0;
    char * path = 0;

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-') {
            if (argv[i][1] == 'a') {
                errno = 0;
                waitAnswers =  strtol(argv[i] + 2, 0, 10);
                if (errno != 0 || waitAnswers == 0) {
                    waitAnswers = 1;
                }
            } else if (argv[i][1] == 't') {
                errno = 0;
                waitSeconds =  strtol(argv[i] + 2, 0, 10);
                if (errno != 0) {
                    ok = 0;
                    break;
                }
            } else {
                ok = 0;
            }
        } else if (path == 0) {
            path = resolved_bus_path(argv[i]);
            argv = argv + i - 1;
            argc = argc - i + 1;
            break;
        }
    }

    if (!path || !ok) {
        fprintf(stderr,"\nusage: ubus-invoke [OPTIONS] /path/to/echo.method <arguments> \n"
                "       -a[N] wait for N answers (default 0. -a without number means 1)\n"
                "       -tN   wait at most N seconds for entire invocation to complete (default infinite) \n");
        exit(1);
    }


    ubus_chan_t *chan = ubus_connect(path);
    if (chan == 0) {
        perror("ubus_connect");
        exit(0);
    }
    int sent = 0;
    int ready = 0;
    int r = 0;
    if ((r = ubus_activate(chan)) == UBUS_ERROR) {
        perror("ubus_activate");
        exit(0);
    }
    if (r == UBUS_CONNECTED)
        ready = 1;

    fd_set rfds;


    alarm(waitSeconds);
    for (;;) {
        if (ready && !sent) {
            sent = 1;
            char *msg = malloc(25001 * argc + 10);
            char *msgp = msg;
            for (int i = 2; i < argc; i++) {
                if (strlen(argv[i]) > 25000) {
                    fprintf(stderr, "excessively large argument\n");
                }
                memcpy(msgp, argv[i], strlen(argv[i]));
                msgp += strlen(argv[i]);
                *msgp++='\t';
            }
            if (*msgp == '\t')
                msgp--;
            *msgp++='\n';
            *msgp++='\0';
            if (ubus_write(chan, msg, strlen(msg)) < 0) {
                perror("send");
                exit(errno);
            }

            if (waitAnswers < 1) {
                exit (0);
            }
        }

        FD_ZERO (&rfds);
        int cfd = ubus_chan_fd(chan);
        FD_SET (cfd, &rfds);
        if (select(cfd + 2, &rfds, NULL, NULL, NULL) < 0) {
            perror("select");
            exit(errno);
        }

        if(FD_ISSET(cfd, &rfds)){
            UBUS_STATUS st = ubus_activate(chan);
            if (st == UBUS_CONNECTED) {
                ready = 1;
            } else if(st == UBUS_READY) {
                char buff [1000];
                int n = ubus_read(chan, &buff, 1000);
                if (n < 1) {
                    exit (0);
                }
                write(1, &buff, n);

                // by ADO spec 2, \n is the end of an answer
                if (strchr(buff, '\n')) {
                    if (--waitAnswers < 1) {
                        exit (0);
                    }
                }
            } else if (st == UBUS_ERROR){
                perror("ubus_activate");
                exit(errno);
            }
        }
    }
    ubus_disconnect(chan);
    return 0;
}

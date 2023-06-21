#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <unistd.h>

#include "log.h"

#define REQUEST_PORT 8000
#define FLOCK_PATH "/server/.~mgr"

enum RequestType
{
    REQUEST_NEWACCOUNT,
    REQUEST_SYNC
};

/* Communication line for handling server requests by unprivilaged users */
int request_fd;

int main(int argc, char *argv[])
{
    // USER MUST RUN AS ROOT
    if (geteuid() != 0)
    {
        printf("Program must be run as root\n");
    }

    // ensure that there is only one manager active at a time
    int lfd = open(FLOCK_PATH, O_CREAT | __O_NOATIME);
    if (flock(lfd, LOCK_EX) != 0)
    {
        printf("Only one isntance of this program can be active at once");
    }

    // invoke setup of error reporting environment
    if (log_setup() != 0)
        exit(1);

    // setup socket vars

    // create socket file descriptors
    if ((request_fd = socket(AF_LOCAL, SOCK_SEQPACKET, 0)) < 0)
        report_error("Could not create socket", MAIN_ERR, CRITICAL);

    // attach socket to specific port

    pthread_t threads[1]; // list of threads that will grow as requests are made
    size_t thread_cnt = 0;
    size_t thread_Cap = 1;

    return 0;
}

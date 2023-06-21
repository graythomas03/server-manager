#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "log.h"

#define COMM_PATH "/tmp/server-request"
#define FLOCK_PATH "/server/.~mgr"

int request_socket_fd; // Socket file descriptor for server-side request management

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

    // setup socket vars //
    struct sockaddr_un request_socket_name;
    size_t request_socket_size;

    // create socket file descriptors
    if ((request_socket_fd = socket(AF_LOCAL, SOCK_SEQPACKET, 0)) < 0)
        report_error("Could not create socket", MAIN_ERR, CRITICAL);

    // assign vfs name to sockets
    request_socket_name.sun_family = AF_LOCAL;
    strncpy(request_socket_name.sun_path, COMM_PATH, sizeof(request_socket_name.sun_path));
    request_socket_name.sun_path[sizeof(request_socket_name.sun_path) - 1] = '\0';
    request_socket_size = (offsetof(struct sockaddr_un, sun_path) + strlen(request_socket_name.sun_path));

    // bind sockets
    if (bind(request_socket_fd, (struct sockaddr *)&request_socket_name, request_socket_size) < 0)
        report_error("Could not bind socket", MAIN_ERR, CRITICAL);

    pthread_t threads[1]; // list of threads that will grow as requests are made
    size_t thread_cnt = 0;
    size_t thread_Cap = 1;

    return 0;
}

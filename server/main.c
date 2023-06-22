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

#define BACKLOG_CNT 10
#define COMM_PATH "/tmp/server-request"
#define FLOCK_PATH "/server/.~mgr"

pthread_t log_thread;

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
    unsigned int request_socket_size;
    // create socket file descriptors
    if ((request_socket_fd = socket(AF_LOCAL, SOCK_SEQPACKET, 0)) < 0)
        report_error(MAIN_ERR, ERR_CRITICAL, "Could not create request socket");
    // assign vfs name to sockets
    request_socket_name.sun_family = AF_LOCAL;
    strncpy(request_socket_name.sun_path, COMM_PATH, sizeof(request_socket_name.sun_path));
    request_socket_name.sun_path[sizeof(request_socket_name.sun_path) - 1] = '\0';
    request_socket_size = (offsetof(struct sockaddr_un, sun_path) + strlen(request_socket_name.sun_path));
    // bind sockets
    if (bind(request_socket_fd, (struct sockaddr *)&request_socket_name, request_socket_size) < 0)
        report_error(MAIN_ERR, ERR_CRITICAL, "Could not bind request socket");

    report_info("Created new Unix Socket %d bound to %s", request_socket_fd, COMM_PATH);

    // start listening to loop sockets
    listen(request_socket_fd, 10);
    // create list of threads to handle server operations
    pthread_t threads[BACKLOG_CNT];
    unsigned int thread_cnt = 0;

    //  socket conneciton queue
    while (1)
    {
        request_socket_size = (offsetof(struct sockaddr_un, sun_path) + strlen(request_socket_name.sun_path));

        // extract first connection
        int connected_socket = accept(request_socket_fd, (struct sockaddr *)&request_socket_name, &request_socket_size);
    }

    return 0;
}

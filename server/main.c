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
#include "request.h"

#define BACKLOG_CNT 10
#define COMM_PATH "/tmp/server-request"
#define FLOCK_PATH "/server/.~mgr"

int request_socket_fd;                  // Socket file descriptor for server-side request management
struct sockaddr_un request_socket_name; // unix socket for local requests via ssh
unsigned int request_socket_size;

/**
 * Function run by local request management thread
 * args -> int representing socket file descriptor
 */
void *local_request_manager(void *args)
{

    // pthread_t threads[backlog_max];
    // int thread_cnt = 0;

    // polling for requests
    while (1)
    {
        // pull new connection from queue
        int connected_socket = accept(request_socket_fd, (struct sockaddr *)&request_socket_name, &request_socket_size);

        //     // ensure there are availible threads
        //     /* TODO: brute force thread management, come up with better solution */
        //     if (thread_cnt >= backlog_max)
        //     {
        //         for (int i = 0; i < backlog_max; ++i)
        //             pthread_join(threads[i], NULL);

        //         thread_cnt = 0;
        //     }

        //     pthread_create(&threads[thread_cnt], NULL, parse_request, &connected_socket);
        //     thread_cnt++;

        // try creating thread as detachable
        // all errors will be written to the log anyways, no need for driver to handle them
        pthread_t newthread;
        if (pthread_create(&newthread, NULL, parse_request, &connected_socket) != 0)
        {
            report_error(THREAD_ERR, ERR_STANDARD, "Error creating process thread for request id %d; Handling in main thread instead", 0);
        }
        else if (pthread_detach(newthread) != 0)
        {
            report_error(THREAD_ERR, ERR_STANDARD, "Error detaching process thread for request id %d", 0);
        }
    }

    return NULL;
}

int main(int argc, char *argv[])
{
    // USER MUST RUN AS ROOT
    if (geteuid() != 0)
    {
        printf("Program must be run as root\n");
        exit(1);
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

    // determine max number of threads/socket backlog for each request management thread
    // may change during dev time
    int backlog_max = BACKLOG_CNT;

    // start listening to sockets
    listen(request_socket_fd, BACKLOG_CNT);

    // pass through sockets to dedicated listening threads
    local_request_manager(NULL);

    return 0;
}

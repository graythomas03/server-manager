#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>

#include "blacklist.h"
#include "request.h"

#define BACKLOG_CNT 10 // maxmimum number of connections that will be held in the queue at anytime

// Default Path Macros
#define COMM_PATH "/tmp/server-request"
#define FLOCK_PATH "/server/.~mgr"
#define CONF_DIR "/etc/server-manager/"
#define CACHE_DIR "/var/cache/server-manager/"

static int request_socket_fd;                  // Socket file descriptor for server-side request management
static struct sockaddr_un request_socket_name; // unix socket for local requests via ssh
static unsigned int request_socket_size;

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

        // try creating thread as detachable
        // all errors will be written to the log anyways, no need for driver to handle them
        pthread_t newthread;
        if (pthread_create(&newthread, NULL, parse_request, &connected_socket) != 0)
        {
            // thread err
        }
        else if (pthread_detach(newthread) != 0)
        {
            // thread err
        }
    }

    return NULL;
}

int main(int argc, char *argv[])
{
    // MUST RUN AS ROOT
    if (geteuid() != 0)
        ;
    {
        printf("Must be run as root\n");
        exit(1);
    }

    // ensure that there is only one manager active at a time
    int lfd = open(FLOCK_PATH, O_CREAT | __O_NOATIME);
    if (flock(lfd, LOCK_EX) != 0)
    {
        printf("Only one isntance of this program can be active at once");
    }

    // start logging program (will likely be TinyLogger)

    // setup socket vars //

    // create socket file descriptors
    if ((request_socket_fd = socket(AF_LOCAL, SOCK_SEQPACKET, 0)) < 0)
        ;
    // socket err

    // Create Server comm Sockets //

    // Local Comm Socket //
    // assign vfs name to sockets
    request_socket_name.sun_family = AF_LOCAL;
    strncpy(request_socket_name.sun_path, COMM_PATH, sizeof(request_socket_name.sun_path));
    request_socket_name.sun_path[sizeof(request_socket_name.sun_path) - 1] = '\0';
    request_socket_size = (offsetof(struct sockaddr_un, sun_path) + strlen(request_socket_name.sun_path));
    // bind sockets
    if (bind(request_socket_fd, (struct sockaddr *)&request_socket_name, request_socket_size) < 0)
        ; // socket err

    // write newly created socket id to log

    // determine max number of threads/socket backlog for each request management thread
    // may change during dev time
    int backlog_max = BACKLOG_CNT;

    // start listening to sockets
    listen(request_socket_fd, BACKLOG_CNT);

    // pass through connected sockets to dedicated listening threads
    local_request_manager(NULL);

    return 0;
}

/****************************************
 * 32-bit implementation
 ****************************************/
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "log.h"

// macros to help with string length calculations
// these will eventually be delegated to a config file
#define DIR_PATH_LEN 13 // length of containing directory /server/logs/
#define LOG_PATH_LEN 29
#define TIME_STR_LEN 14
// default capacity for each string buffer
#define LOG_STR_BUFFER_LEN 100
#define YEAR_OFFSET_BASE 1900 // used to derive current year from offset

/**
 * Each entry in the queue will contain a string to print
 * and a va_list
 */
struct WriteQEntry
{
    int len;
    char *str;
};
/* Log Shutdown Indicator */
static char destroy = 0;
/* File Descriptor of current log file */
int log_fd;
/* Log entry write queue */
static struct WriteQEntry *write_queue;
static int queue_front;
static int queue_rear;
static int queue_cap;
static int queue_size = 0;
/* Log Writer Thread and Mutex */
static pthread_t write_thread;
static pthread_mutex_t write_queue_lock = PTHREAD_MUTEX_INITIALIZER;
// pthread_cond_t write_queue_full = PTHREAD_COND_INITIALIZER;
static pthread_cond_t write_queue_avail = PTHREAD_COND_INITIALIZER;

// helper method for size managemFILEent
static inline void *check_buffer_resize(void *victim, int *current, int needed)
{
    if (*current > needed)
        return victim;

    // double capacity
    *current = *current << 2;
    if (*current < needed)
        *current = needed;

    return realloc(victim, *current);
}

static inline int digit_cnt(int src)
{
    int digits = 0;
    do
    {
        src /= 10;
        digits++;
    } while (src != 0);

    return digits;
}

static void *queue_writer(void *arg)
{
    // expect threshold argument
    int threshold = *((int *)arg);

    // this loop will continue until program ends
    while (destroy == 0)
    {
        // only start writing once queue reaches designated threshold
        // this should reduce amount of actual writing done to improve performance
        while (queue_size >= threshold)
        {
            pthread_mutex_lock(&write_queue_lock);
            // pull form queue
            struct WriteQEntry victim = write_queue[queue_front];
            queue_front = (queue_front + 1) % queue_cap;
            --queue_size;
            // out of critical section, can active other threads that need the write queue
            pthread_mutex_unlock(&write_queue_lock);
            pthread_cond_signal(&write_queue_avail);

            // write log string to entry
            write(log_fd, victim.str, victim.len);
            free(victim.str);
        }
    }

    // if we reach this point, program is shutting down
    // so all logs must be written
    while (queue_size > 0)
    {
        struct WriteQEntry victim = write_queue[queue_front];
        queue_front = (queue_front + 1) % queue_cap;
        --queue_size;
        write(log_fd, victim.str, victim.len);
    }

    return NULL;
}

static void *append_queue(void *args)
{
    struct WriteQEntry tmp = *((struct WriteQEntry *)args);

    // acquire a lock on the write queue
    pthread_mutex_lock(&write_queue_lock);
    // queue is full, wait for it to lower a bit
    if (queue_size >= queue_cap)
    {
        pthread_cond_wait(&write_queue_avail, &write_queue_lock);
    }
    // add new log entry to queue
    write_queue[queue_rear] = tmp;
    queue_rear = (queue_rear + 1) % queue_cap;
    ++queue_size;

    pthread_mutex_unlock(&write_queue_lock);

    return NULL;
}

/** Writes formatted strings to the log file*/
inline void log_write(const char *str, va_list args)
{
    // using output buffer to reduce amount of fs write calls
    int buffer_cap = TIME_STR_LEN + LOG_STR_BUFFER_LEN;
    int buffer_cnt = 0;
    char *out_buffer = malloc(buffer_cap);

    // misc variables
    union arg
    {
        int i;
        double f;
        char *s;
    };

    union arg tmp_0;
    union arg tmp_1;

    // basically a simple printf
    while (*str != '\0')
    {
        switch (*str)
        {
        case '%':
            ++str; // determine which flag
            switch (*str)
            {
            case 'd':
                tmp_0.i = va_arg(args, int); // integer
                out_buffer = check_buffer_resize(out_buffer, &buffer_cap, buffer_cnt + digit_cnt(tmp_0.i));
                buffer_cnt += sprintf(out_buffer + buffer_cnt, "%d", tmp_0.i);
                break;
            case 's':
                tmp_0.s = va_arg(args, char *); // pointer to string value
                tmp_1.i = strlen(tmp_0.s);      // length of string value
                out_buffer = check_buffer_resize(out_buffer, &buffer_cap, buffer_cnt + tmp_1.i);
                memcpy(out_buffer + buffer_cnt, tmp_0.s, tmp_1.i);
                buffer_cnt += tmp_1.i;
                break;
            case 'f':
                // not yet implemented
                break;
            }
            break;
        case '\\':
            // skip to next character
            ++str;
            // special new line and tab cases
            if (*str == 'n')
            {
                *(out_buffer + buffer_cnt) = '\n';
                ++buffer_cnt;
                break;
            }
            else if (*str == 't')
            {
                *(out_buffer + buffer_cnt) = '\t';
                ++buffer_cnt;
                break;
            }
            // other characters will fall through to print as normal
        default:
            out_buffer = check_buffer_resize(out_buffer, &buffer_cap, buffer_cnt + 1);
            *(out_buffer + buffer_cnt) = *str;
            ++buffer_cnt;
        }

        ++str;
    }

    // write terminating newline
    *(out_buffer + buffer_cnt) = '\n';
    ++buffer_cnt;

    // append to queue
    struct WriteQEntry tmp = {buffer_cnt, out_buffer};
    pthread_t newthread;
    pthread_create(&newthread, NULL, append_queue, &tmp);
    pthread_detach(newthread);
}

int log_setup(int write_qcap, int write_qthreshold)
{
    // generate filename based on current date
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    /*
     *
     * File time string format
     * server.log.*year*.*month*.*day*
     * server.log.2023.06.15
     *
     */
    short year = tm.tm_year + YEAR_OFFSET_BASE;
    short month = tm.tm_mon + 1;
    short day = tm.tm_mday;
    // open or create log file
    char log_path[LOG_PATH_LEN + 1];
    // hostname will be replaced later by a config file system
    sprintf(log_path, "hostname.log.%04d.%02d.%02d", year, month, day);
    open(log_path, O_CREAT | O_APPEND);
    // allocate circular write buffer
    queue_cap = write_qcap;
    queue_front = 0;
    queue_rear = 0;
    write_queue = calloc(write_qcap, sizeof(struct WriteQEntry));
    // create file writer thread
    int threshold = write_qthreshold;
    pthread_create(&write_thread, NULL, queue_writer, &threshold);
    pthread_detach(write_thread);
    // success state
    return 0;
}

void log_destroy()
{
    destroy = 1; // set destroy to let all log threads know their time has come
    // might do errno stuff here //
    // wait until writing queue is empty
    while (queue_size > 0)
        ;

    // close log file
    close(log_fd);
}

void report_info(const char *str, ...)
{

    va_list args;
    va_start(args, str);

    log_write(str, args);
}

void report_error(enum ErrorOrigin error_origin, enum ErrorLevel error_level, const char *error_str, ...)
{

    int len = 0, origin_len = 0, level_len = 0;
    char *levelstr, *originstr;
    switch (error_level)
    {
    case ERR_CRITICAL:
        levelstr = "CRITICAL: "; // str is len 10
        len += 10;
        break;
    case ERR_STANDARD:
        levelstr = "STANDARD: "; // str is len 10
        len += 10;
        break;
    }

    switch (error_origin)
    {
    case ACCOUNT_ERR:
        originstr = " ACCOUNT_ERR-> ";
        len += 14;
        break;
    case SYNC_ERR:
        originstr = " SYNC_ERR-> ";
        len += 12;
        break;
    case MAIN_ERR:
        originstr = " DRIVER_ERR-> ";
        len += 14;
        break;

    case THREAD_ERR:
        originstr = "THREAD_ERR-> ";
        len += 13;
        break;
    }

    // combine all strings
    len += strlen(error_str);
    char final_error_str[len + 1];
    int final_idx = 0;

    while (*levelstr != '\0')
    {
        final_error_str[final_idx++] = *levelstr;
        ++levelstr;
    }
    while (*originstr != '\0')
    {
        final_error_str[final_idx++] = *originstr;
        ++originstr;
    }
    while (*error_str != '\0')
    {
        final_error_str[final_idx++] = *error_str;
        ++originstr;
    }

    // write error string using log_write
    va_list args;
    va_start(args, error_str);

    log_write(final_error_str, args);
};
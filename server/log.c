#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "log.h"

#define LOGSTR_LEN 29

static FILE *log_fp;

int log_setup()
{

    /* create new log file for this instance */
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

    short year = tm.tm_year - 1900;
    short month = tm.tm_mon + 1;
    short day = tm.tm_mday;

    current_logfile = malloc(LOGSTR_LEN * sizeof(char));
    snprintf(current_logfile, LOGSTR_LEN, "/server/server.log.%04d.%02d.%02d", year, month, day);

    // finall create new file
    log_fp = fopen(current_logfile, "w+");

    // if log file cant be opened, return -1
    if (log_fp == NULL)
        return 1;

    // success state
    return 0;
}

int log_destroy()
{
    int status = 0;

    // close log file
    status |= fclose(log_fp);
    free(current_logfile);

    return status;
}

// helper method for size management
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
/** Writes formatted strings to the log file*/
static void log_write(const char *str, va_list args)
{
    // using output buffer to reduce amount of fs write calls
    int buffer_cap = 10;
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
            *(out_buffer + buffer_cnt) = *str;
            ++buffer_cnt;
        }

        ++str;
    }

    // write terminating newline
    *(out_buffer + buffer_cnt) = '\n';
    ++buffer_cnt;

    // string has been read, block write to log file
    fwrite(out_buffer, buffer_cnt, 1, log_fp);

    // clean up dynamic memory
    free(out_buffer);
}

void report_info(const char *str, ...)
{
    va_list args;
    va_start(args, str);

    log_write(str, args);
}

void report_error(enum ErrorOrigin error_origin, enum ErrorLevel error_level, const char *error_str, ...)
{

    int fail = 0; // used to determine if this error should immidiately close the program

    char *levelstr, *originstr;
    int level_len = 0, origin_len = 0;
    switch (error_level)
    {
    case ERR_CRITICAL:
        fail |= 1;
        levelstr = "CRITICAL: "; // str is len 10
        level_len = 10;
        break;
    case ERR_STANDARD:
        levelstr = "STANDARD: "; // str is len 10
        level_len = 10;
        break;
    default:
        levelstr = ": ";
        level_len = 2;
    }

    switch (error_origin)
    {
    case ACCOUNT_ERR:
        originstr = " ACCOUNT_ERR-> ";
        origin_len = 14;
        break;
    case SYNC_ERR:
        originstr = " SYNC_ERR-> ";
        origin_len = 12;
        break;
    case MAIN_ERR:
        originstr = " DRIVER_ERR-> ";
        origin_len = 14;
        break;
    }

    // write level string
    while (level_len)
    {
        fputc(*levelstr, log_fp);
        levelstr++;
    }
    // write error origin string
    while (origin_len)
    {
        fputc(*originstr, log_fp);
        originstr++;
    }

    // write error string using log_write
    va_list args;
    va_start(args, error_str);

    log_write(error_str, args);

    if (fail != 0)
    {
        log_destroy();
        exit(1);
    }
}

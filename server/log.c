#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "log.h"

#define LOGSTR_LEN 29

FILE *log_fp;

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

void report_error(char *error_str, enum ErrorOrigin error_origin, enum ErrorLevel error_level)
{
    int fail = 0; // used to determine if this error should immidiately close the program

    char *levelstr, *originstr;
    int level_len = 0, origin_len = 0;
    switch (error_level)
    {
    case CRITICAL:
        fail |= 1;
        levelstr = "CRITICAL: "; // str is len 10
        level_len = 10;
        break;
    case STANDARD:
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

    // write error
    fprintf(log_fp, "%s\n", error_str);

    if (fail != 0)
    {
        log_destroy();
        exit(1);
    }
}

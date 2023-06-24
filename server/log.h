#ifndef _LOG_H
#define _LOG_H

#include <stdarg.h>

enum ErrorLevel
{
    ERR_STANDARD = 0,
    ERR_CRITICAL
};
enum ErrorOrigin
{
    ACCOUNT_ERR,
    SYNC_ERR,
    MAIN_ERR,
    THREAD_ERR
};

int log_setup(int write_qcap, int write_qthreshold);

void log_destroy();

void report_info(const char *str, ...);

void report_error(enum ErrorOrigin error_origin, enum ErrorLevel error_level, const char *error_str, ...);

#endif

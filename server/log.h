#ifndef __LOG__
#define __LOG__

enum ErrorLevel
{
    STANDARD = 0,
    CRITICAL
};
enum ErrorOrigin
{
    ACCOUNT_ERR,
    SYNC_ERR,
    MAIN_ERR
};

extern char *current_logfile;

int log_setup();

int log_destroy();

void report_error(char *error_str, enum ErrorOrigin, enum ErrorLevel);

#endif

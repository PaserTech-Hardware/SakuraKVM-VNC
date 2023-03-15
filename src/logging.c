#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>

#include "logging.h"
#include "sakurakvmd.h"

LOG_LEVEL g_log_level = LOG_LEVEL_INFO;
FILE *g_log_file = NULL;

const char *log_level_to_prefix(LOG_LEVEL level)
{
    switch (level) {
        case LOG_LEVEL_VERBOSE:
            return LOG_VERBOSE;
        case LOG_LEVEL_DEBUG:
            return LOG_DEBUG;
        case LOG_LEVEL_INFO:
            return LOG_INFO;
        case LOG_LEVEL_WARN:
            return LOG_WARN;
        case LOG_LEVEL_ERROR:
            return LOG_ERROR;
        default:
            return "[UNKNOWN]";
    }
}

const char *log_level_to_color(LOG_LEVEL level)
{
    switch (level) {
        case LOG_LEVEL_VERBOSE:
            return LOG_COLOR_GRAY;
        case LOG_LEVEL_DEBUG:
            return LOG_COLOR_CYAN;
        case LOG_LEVEL_INFO:
            return LOG_COLOR_GREEN;
        case LOG_LEVEL_WARN:
            return LOG_COLOR_YELLOW;
        case LOG_LEVEL_ERROR:
            return LOG_COLOR_RED;
        default:
            return "";
    }
}

void log_msg(LOG_LEVEL level, const char *module_name, const char *func_name, unsigned long linenumber, const char *fmt, ...)
{
    char time_str[32];
    time_t now = 0;

    if(level < g_log_level)
    {
        return;
    }

    now = time(NULL);
    strftime(time_str, sizeof(time_str), LOG_TIME_FMT, localtime(&now));

    va_list args;
    va_start(args, fmt);
    // console print part
    printf("%s%s[%s]%s ", log_level_to_color(level), time_str, module_name, log_level_to_prefix(level));
    vprintf(fmt, args);
    if(g_log_level <= LOG_LEVEL_DEBUG)
        printf(" (%s:%lu)", func_name, linenumber);
    printf("\n");
    va_end(args);
    
    va_start(args, fmt);
    // file print part
    if(g_log_file == NULL)
    {
        goto log_end;
    }
    fprintf(g_log_file, "%s[%s]%s ", time_str, module_name, log_level_to_prefix(level));
    vfprintf(g_log_file, fmt, args);
    if(g_log_level <= LOG_LEVEL_DEBUG)
        fprintf(g_log_file, " (%s:%lu)", func_name, linenumber);
    fprintf(g_log_file, "\n");

    log_end:
    va_end(args);
}

int logging_init(LOG_LEVEL log_level)
{
    int errorno_ret;
    const char *log_path = LOG_PATH;
    g_log_level = log_level;
    
    g_log_file = fopen(log_path, "a");
    errorno_ret = errno;
    if(g_log_file == NULL)
    {
        LOGE("Failed to open log file %s: %s", log_path, strerror(errorno_ret));
        return errorno_ret;
    }

    LOGI("SakuraKVM Daemon - Version: %s", SAKURAKVMD_VERSION_ALL);
    LOGI("Copyright (C) 2023 PaserTech SakuraKVM Project");
    LOGI("Logging initialized, set log level to %s", log_level_to_prefix(log_level));

    return 0;
}

void logging_close()
{
    LOGI("Logging closed");
    if(g_log_file != NULL)
    {
        fclose(g_log_file);
        g_log_file = NULL;
    }

    // reset the terminal color
    printf("%s", LOG_COLOR_RESET);

    return;
}

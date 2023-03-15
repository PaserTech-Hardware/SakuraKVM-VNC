#ifndef __SAKURAKVMD_LOGGING_H__
#define __SAKURAKVMD_LOGGING_H__

#define LOG_PATH "/var/log/sakurakvmd.log"

#define LOG_COLOR_GRAY "\033[1;30m"
#define LOG_COLOR_CYAN "\033[1;36m"
#define LOG_COLOR_GREEN "\033[1;32m"
#define LOG_COLOR_YELLOW "\033[1;33m"
#define LOG_COLOR_RED "\033[1;31m"
#define LOG_COLOR_RESET "\033[0m"

#define LOG_VERBOSE "[VERBOSE]"
#define LOG_DEBUG "[DEBUG]"
#define LOG_INFO "[INFO]"
#define LOG_WARN "[WARN]"
#define LOG_ERROR "[ERROR]"

#define LOG_TIME_FMT "[%Y-%m-%d %H:%M:%S]"

#define LOGV(...) log_msg(LOG_LEVEL_VERBOSE, __FILE__, __func__, __LINE__ , ##__VA_ARGS__)
#define LOGD(...) log_msg(LOG_LEVEL_DEBUG, __FILE__, __func__, __LINE__ , ##__VA_ARGS__)
#define LOGI(...) log_msg(LOG_LEVEL_INFO, __FILE__, __func__, __LINE__ , ##__VA_ARGS__)
#define LOGW(...) log_msg(LOG_LEVEL_WARN, __FILE__, __func__, __LINE__ , ##__VA_ARGS__)
#define LOGE(...) log_msg(LOG_LEVEL_ERROR, __FILE__, __func__, __LINE__ , ##__VA_ARGS__)

enum _LOG_LEVEL {
    LOG_LEVEL_VERBOSE = 0,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_MAX,
};
typedef enum _LOG_LEVEL LOG_LEVEL;

/*
* Initalize the logging system, open log file and set the default log level.
*
* @return linux errorno code
*/
int logging_init(LOG_LEVEL log_level);

/*
* Logging function, use this function to log message.
*
* @param level log level
* @param module_name module name
* @param func_name function name
* @param linenumber line number
* @param fmt format string
* @param ... variable arguments
*
* @return linux errorno code
*/
void log_msg(LOG_LEVEL level, const char *module_name, const char *func_name, unsigned long linenumber, const char *fmt, ...);

/*
* End the logging system, close log file.
*/
void logging_close();

#endif

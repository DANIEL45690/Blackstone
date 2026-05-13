#ifndef P2P_CORE_LOGGER_H
#define P2P_CORE_LOGGER_H

#include "../include/common.h"

void logger_init(void);
void logger_shutdown(void);
void logger_set_level(log_level_t level);
void logger_set_console(int enable);
void logger_set_file(const char *path);
void logger_log(log_level_t level, const char *file, int line, const char *fmt, ...);

#define LOG_DEBUG(...) logger_log(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_INFO(...) logger_log(LOG_LEVEL_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARN(...) logger_log(LOG_LEVEL_WARN, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(...) logger_log(LOG_LEVEL_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_FATAL(...) logger_log(LOG_LEVEL_FATAL, __FILE__, __LINE__, __VA_ARGS__)

#endif

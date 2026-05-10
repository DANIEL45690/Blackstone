#ifndef BANK_LOG_H
#define BANK_LOG_H

#include "bank_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

    void bank_log(bank_log_level level, const char *file, int line, const char *fmt, ...);
    void bank_set_log_level(bank_log_level level);
    int bank_set_log_file(const char *filename);
    void bank_close_log_file(void);
    void bank_log_hexdump(bank_log_level level, const uint8_t *data, size_t len, const char *label);

#define BANK_LOG_DEBUG(...) bank_log(BANK_LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
#define BANK_LOG_INFO(...) bank_log(BANK_LOG_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define BANK_LOG_WARN(...) bank_log(BANK_LOG_WARN, __FILE__, __LINE__, __VA_ARGS__)
#define BANK_LOG_ERROR(...) bank_log(BANK_LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define BANK_LOG_FATAL(...) bank_log(BANK_LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif

#include "bank/bank_log.h"
#include "bank/bank_platform.h"
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

static bank_log_level g_log_level = BANK_LOG_INFO;
static FILE *g_log_file = NULL;

void bank_set_log_level(bank_log_level level)
{
    g_log_level = level;
}

int bank_set_log_file(const char *filename)
{
    if (g_log_file)
        fclose(g_log_file);
    g_log_file = fopen(filename, "a");
    if (!g_log_file)
        return 0;
    setvbuf(g_log_file, NULL, _IONBF, 0);
    return 1;
}

void bank_close_log_file(void)
{
    if (g_log_file)
        fclose(g_log_file);
    g_log_file = NULL;
}

void bank_log(bank_log_level level, const char *file, int line, const char *fmt, ...)
{
    if (level < g_log_level)
        return;

    const char *level_str[] = {"DEBUG", "INFO", "WARN", "ERROR", "FATAL"};
    char buffer[BANK_LOG_BUFFER_SIZE];
    va_list args;
    time_t now;
    struct tm *tm_info;

    time(&now);
    tm_info = localtime(&now);

    va_start(args, fmt);
    vsnprintf(buffer, BANK_LOG_BUFFER_SIZE, fmt, args);
    va_end(args);

    if (g_log_file)
    {
        fprintf(g_log_file, "[%04d-%02d-%02d %02d:%02d:%02d] [%s] [%s:%d] %s\n",
                tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
                tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec,
                level_str[level], file, line, buffer);
        fflush(g_log_file);
    }
    else
    {
        fprintf(stderr, "[%s] %s\n", level_str[level], buffer);
    }
}

void bank_log_hexdump(bank_log_level level, const uint8_t *data, size_t len, const char *label)
{
    if (level < g_log_level)
        return;

    char hex_buffer[80];
    size_t offset = 0;

    if (label)
        bank_log(level, __FILE__, __LINE__, "Hexdump: %s (%zu bytes)", label, len);

    for (size_t i = 0; i < len; i += 16)
    {
        offset = 0;
        offset += snprintf(hex_buffer + offset, sizeof(hex_buffer) - offset, "%04zx: ", i);

        for (size_t j = 0; j < 16 && i + j < len; j++)
        {
            offset += snprintf(hex_buffer + offset, sizeof(hex_buffer) - offset, "%02x ", data[i + j]);
        }

        for (size_t j = 16 - ((len - i) % 16); j < 16 && (len - i) < 16; j++)
        {
            offset += snprintf(hex_buffer + offset, sizeof(hex_buffer) - offset, "   ");
        }

        offset += snprintf(hex_buffer + offset, sizeof(hex_buffer) - offset, " |");

        for (size_t j = 0; j < 16 && i + j < len; j++)
        {
            char c = data[i + j];
            if (c >= 32 && c <= 126)
                offset += snprintf(hex_buffer + offset, sizeof(hex_buffer) - offset, "%c", c);
            else
                offset += snprintf(hex_buffer + offset, sizeof(hex_buffer) - offset, ".");
        }

        offset += snprintf(hex_buffer + offset, sizeof(hex_buffer) - offset, "|");

        bank_log(level, __FILE__, __LINE__, "%s", hex_buffer);
    }
}

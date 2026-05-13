#include "logger.h"
#include <windows.h>

static struct
{
    log_level_t level;
    FILE *file;
    CRITICAL_SECTION lock;
    int console_output;
    char log_path[256];
} g_logger = {0};

static int g_initialized = 0;

void logger_init(void)
{
    if (g_initialized)
        return;

    memset(&g_logger, 0, sizeof(g_logger));
    g_logger.level = LOG_LEVEL_DEBUG;
    g_logger.console_output = 1;
    strcpy(g_logger.log_path, "logs/p2p_system.log");

    InitializeCriticalSection(&g_logger.lock);

    g_logger.file = fopen(g_logger.log_path, "w");
    if (g_logger.file)
    {
        time_t now = time(NULL);
        fprintf(g_logger.file, "=== P2P SECURITY SYSTEM LOG v%d.%d.%d ===\n",
                VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
        fprintf(g_logger.file, "Started: %s", ctime(&now));
        fprintf(g_logger.file, "=========================================\n\n");
        fflush(g_logger.file);
    }

    g_initialized = 1;
}

void logger_shutdown(void)
{
    if (!g_initialized)
        return;

    EnterCriticalSection(&g_logger.lock);
    if (g_logger.file)
    {
        time_t now = time(NULL);
        fprintf(g_logger.file, "\n=== LOG CLOSED: %s", ctime(&now));
        fclose(g_logger.file);
        g_logger.file = NULL;
    }
    LeaveCriticalSection(&g_logger.lock);

    DeleteCriticalSection(&g_logger.lock);
    g_initialized = 0;
}

void logger_set_level(log_level_t level)
{
    EnterCriticalSection(&g_logger.lock);
    g_logger.level = level;
    LeaveCriticalSection(&g_logger.lock);
}

void logger_set_console(int enable)
{
    EnterCriticalSection(&g_logger.lock);
    g_logger.console_output = enable;
    LeaveCriticalSection(&g_logger.lock);
}

void logger_set_file(const char *path)
{
    EnterCriticalSection(&g_logger.lock);
    strcpy(g_logger.log_path, path);
    if (g_logger.file)
    {
        fclose(g_logger.file);
    }
    g_logger.file = fopen(path, "a");
    LeaveCriticalSection(&g_logger.lock);
}

void logger_log(log_level_t level, const char *file, int line, const char *fmt, ...)
{
    if (!g_initialized || level < g_logger.level)
        return;

    const char *level_str[] = {"DEBUG", "INFO", "WARN", "ERROR", "FATAL"};
    char buffer[4096];
    va_list args;
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);

    va_start(args, fmt);
    vsnprintf(buffer, sizeof(buffer), fmt, args);
    va_end(args);

    EnterCriticalSection(&g_logger.lock);

    if (g_logger.console_output)
    {
        printf("[%04d-%02d-%02d %02d:%02d:%02d] [%s] [%s:%d] %s\n",
               tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
               tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec,
               level_str[level], file, line, buffer);
    }

    if (g_logger.file)
    {
        fprintf(g_logger.file, "[%04d-%02d-%02d %02d:%02d:%02d] [%s] [%s:%d] %s\n",
                tm_info->tm_year + 1900, tm_info->tm_mon + 1, tm_info->tm_mday,
                tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec,
                level_str[level], file, line, buffer);
        fflush(g_logger.file);
    }

    LeaveCriticalSection(&g_logger.lock);
}

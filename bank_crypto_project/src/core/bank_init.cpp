#include "bank/bank_crypto.h"
#include "bank/bank_platform.h"
#include "bank/bank_rng.h"
#include <string.h>

extern bank_crypto_context g_ctx;
extern bank_session_manager g_session_mgr;
extern bank_rng_state g_rng_state;
extern int g_library_initialized;
extern uint64_t g_library_start_time;

#if defined(_WIN32) || defined(_WIN64)
extern CRITICAL_SECTION g_mutex;
extern CRITICAL_SECTION g_rng_mutex;
extern CRITICAL_SECTION g_session_mutex;
#else
extern pthread_mutex_t g_mutex;
extern pthread_mutex_t g_rng_mutex;
extern pthread_mutex_t g_session_mutex;
#endif

void bank_context_init(void)
{
    memset(&g_ctx, 0, sizeof(g_ctx));
    memset(&g_session_mgr, 0, sizeof(g_session_mgr));
    memset(&g_rng_state, 0, sizeof(g_rng_state));
    g_library_initialized = 0;
    g_library_start_time = 0;
}

bank_crypto_context *bank_get_context(void)
{
    return &g_ctx;
}

bank_session_manager *bank_get_session_manager(void)
{
    return &g_session_mgr;
}

bank_rng_state *bank_get_rng_state(void)
{
    return &g_rng_state;
}

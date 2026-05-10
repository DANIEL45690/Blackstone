#include "bank/bank_crypto.h"
#include "bank/bank_platform.h"
#include <string.h>

extern bank_crypto_context g_ctx;
extern bank_session_manager g_session_mgr;
extern bank_rng_state g_rng_state;
extern int g_library_initialized;
extern uint64_t g_library_start_time;

int bank_init(void)
{
    if (g_library_initialized)
        return 1;

    bank_platform_init();

    if (!bank_init_master_random())
        return 0;

    return 1;
}

void bank_shutdown(void)
{
    if (!g_library_initialized)
        return;

    bank_wipe_master();
    bank_platform_cleanup();
}

#include "crypto_api.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static CryptoContext g_ctx;

int main(int argc, char **argv)
{
    int result;

    result = crypto_init(&g_ctx);
    if (result != CRYPTO_SUCCESS)
    {
        printf("Crypto init failed\n");
        return 1;
    }

    if (argc > 1 && strcmp(argv[1], "--test") == 0)
    {
        result = crypto_self_test(&g_ctx);
        if (result == CRYPTO_SUCCESS)
        {
            printf("Self test passed\n");
        }
        else
        {
            printf("Self test failed\n");
            crypto_shutdown(&g_ctx);
            return 1;
        }
    }
    else
    {
        printf("P2P Crypto System Ready\n");
        printf("Security status: 0x%llX\n", get_security_status(&g_ctx));
    }

    crypto_shutdown(&g_ctx);
    return 0;
}

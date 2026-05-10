#include <stdio.h>
#include <string.h>
#include <time.h>
#include "bank/bank_aes.h"

#define BENCH_ITERATIONS 100000

void bench_aes_encrypt(void)
{
    bank_aes256_key key;
    uint8_t user_key[32];
    uint8_t plain[16];
    uint8_t cipher[16];

    for (int i = 0; i < 32; i++)
        user_key[i] = i;
    for (int i = 0; i < 16; i++)
        plain[i] = i;

    bank_aes256_set_key(&key, user_key);

    clock_t start = clock();
    for (int i = 0; i < BENCH_ITERATIONS; i++)
        bank_aes_encrypt_block(plain, cipher, &key);
    clock_t end = clock();

    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
    printf("AES-256 encrypt: %d ops in %.3f sec (%.0f ops/sec)\n",
           BENCH_ITERATIONS, time_spent, BENCH_ITERATIONS / time_spent);
}

int main(void)
{
    printf("\n=== AES Performance ===\n");
    bench_aes_encrypt();
    return 0;
}

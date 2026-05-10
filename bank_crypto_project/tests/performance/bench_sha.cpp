#include <stdio.h>
#include <string.h>
#include <time.h>
#include "bank/bank_sha.h"

#define BENCH_ITERATIONS 100000
#define DATA_SIZE 1024

void bench_sha256(void)
{
    uint8_t data[DATA_SIZE];
    uint8_t hash[32];

    for (int i = 0; i < DATA_SIZE; i++)
        data[i] = i & 0xFF;

    clock_t start = clock();
    for (int i = 0; i < BENCH_ITERATIONS; i++)
        bank_sha256(data, DATA_SIZE, hash);
    clock_t end = clock();

    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;
    printf("SHA-256: %d ops in %.3f sec (%.0f ops/sec)\n",
           BENCH_ITERATIONS, time_spent, BENCH_ITERATIONS / time_spent);
}

int main(void)
{
    printf("\n=== SHA Performance ===\n");
    bench_sha256();
    return 0;
}

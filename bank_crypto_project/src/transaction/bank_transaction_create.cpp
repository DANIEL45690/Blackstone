#include "bank/bank_transaction.h"
#include "bank/bank_hmac.h"
#include "bank/bank_sha.h"
#include "bank/bank_rng.h"
#include "bank/bank_log.h"
#include <string.h>

int bank_create_transaction(uint64_t tx_id, const uint8_t *from, const uint8_t *to,
                            uint64_t amount, const uint8_t *currency,
                            int session_id, bank_transaction *tx)
{
    if (!from || !to || !currency || !tx)
        return 0;

    memset(tx, 0, sizeof(bank_transaction));

    tx->transaction_id = tx_id;
    tx->timestamp = time(NULL);
    memcpy(tx->from_account, from, 32);
    memcpy(tx->to_account, to, 32);
    tx->amount = amount;
    memcpy(tx->currency, currency, 8);

    if (!bank_rng_bytes((uint8_t *)&tx->nonce, 8))
        return 0;

    uint8_t data[512];
    size_t data_len = 0;
    memcpy(data + data_len, &tx->transaction_id, 8);
    data_len += 8;
    memcpy(data + data_len, &tx->timestamp, 8);
    data_len += 8;
    memcpy(data + data_len, tx->from_account, 32);
    data_len += 32;
    memcpy(data + data_len, tx->to_account, 32);
    data_len += 32;
    memcpy(data + data_len, &tx->amount, 8);
    data_len += 8;
    memcpy(data + data_len, tx->currency, 8);
    data_len += 8;
    memcpy(data + data_len, &tx->nonce, 8);
    data_len += 8;

    extern int bank_sign_data(int session_id, const uint8_t *data, size_t data_len, uint8_t *signature);
    if (!bank_sign_data(session_id, data, data_len, tx->signature))
        return 0;

    bank_sha256(data, data_len, tx->reference_hash);

    tx->verified = 1;

    BANK_LOG_INFO("Created transaction %llu for amount %llu",
                  (unsigned long long)tx_id, (unsigned long long)amount);

    return 1;
}

void bank_transaction_set_metadata(bank_transaction *tx, const uint8_t *metadata, size_t metadata_len)
{
    if (!tx || !metadata || metadata_len > 128)
        return;

    memcpy(tx->metadata, metadata, metadata_len);
    tx->metadata_len = metadata_len;
}

void bank_transaction_clear_metadata(bank_transaction *tx)
{
    if (!tx)
        return;

    bank_secure_zero(tx->metadata, sizeof(tx->metadata));
    tx->metadata_len = 0;
}

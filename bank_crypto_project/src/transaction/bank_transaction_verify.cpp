#include "bank/bank_transaction.h"
#include "bank/bank_hmac.h"
#include "bank/bank_sha.h"
#include "bank/bank_utils.h"
#include "bank/bank_log.h"
#include <string.h>

int bank_verify_transaction(const bank_transaction *tx, int session_id)
{
    if (!tx || !tx->verified)
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

    uint8_t expected_hash[32];
    bank_sha256(data, data_len, expected_hash);

    if (!bank_secure_compare(tx->reference_hash, expected_hash, 32))
        return 0;

    extern int bank_verify_signature(int session_id, const uint8_t *data, size_t data_len, const uint8_t *signature);
    return bank_verify_signature(session_id, data, data_len, tx->signature);
}

int bank_transaction_validate(const bank_transaction *tx)
{
    if (!tx)
        return 0;

    if (tx->amount == 0)
        return 0;

    int from_zero = 1;
    int to_zero = 1;

    for (int i = 0; i < 32; i++)
    {
        if (tx->from_account[i] != 0)
            from_zero = 0;
        if (tx->to_account[i] != 0)
            to_zero = 0;
    }

    if (from_zero || to_zero)
        return 0;

    if (tx->timestamp > time(NULL) + 3600)
        return 0;

    return 1;
}

void bank_transaction_sign(bank_transaction *tx, const uint8_t *key, size_t key_len)
{
    if (!tx || !key)
        return;

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

    bank_hmac_sha256(key, key_len, data, data_len, tx->signature);
    tx->verified = 1;
}   

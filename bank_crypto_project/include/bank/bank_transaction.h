#ifndef BANK_TRANSACTION_H
#define BANK_TRANSACTION_H

#include "bank_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

    int bank_create_transaction(uint64_t tx_id, const uint8_t *from, const uint8_t *to,
                                uint64_t amount, const uint8_t *currency,
                                int session_id, bank_transaction *tx);

    int bank_verify_transaction(const bank_transaction *tx, int session_id);

    void bank_transaction_sign(bank_transaction *tx, const uint8_t *key, size_t key_len);
    int bank_transaction_validate(const bank_transaction *tx);
    void bank_transaction_set_metadata(bank_transaction *tx, const uint8_t *metadata, size_t metadata_len);
    void bank_transaction_clear_metadata(bank_transaction *tx);

#ifdef __cplusplus
}
#endif

#endif

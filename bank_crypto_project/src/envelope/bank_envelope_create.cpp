#include "bank/bank_envelope.h"
#include "bank/bank_session.h"
#include "bank/bank_gcm.h"
#include "bank/bank_sha.h"
#include "bank/bank_utils.h"
#include "bank/bank_rng.h"
#include "bank/bank_log.h"
#include <string.h>

extern int g_library_initialized;
extern bank_crypto_context g_ctx;

static int bank_encrypt_data(const uint8_t *plain, size_t plain_len,
                             const uint8_t *aad, size_t aad_len,
                             int session_id,
                             uint8_t *cipher, size_t *cipher_len,
                             uint8_t iv[BANK_GCM_IV_BYTES],
                             uint8_t tag[BANK_GCM_TAG_BYTES])
{
    if (!g_ctx.initialized)
        return 0;
    if (!plain || !cipher || !cipher_len || !iv || !tag)
        return 0;
    if (plain_len > BANK_MAX_DATA)
        return 0;
    if (!bank_rng_bytes(iv, BANK_GCM_IV_BYTES))
        return 0;

    uint8_t session_key[BANK_AES_KEY_BYTES];
    if (!bank_get_session_key(session_id, session_key, NULL))
        return 0;

    int result = bank_aes256_gcm_encrypt(session_key, iv, BANK_GCM_IV_BYTES,
                                         aad, aad_len, plain, plain_len,
                                         cipher, tag);

    if (result)
    {
        *cipher_len = plain_len;
        g_ctx.total_encryptions++;
    }

    bank_secure_zero(session_key, BANK_AES_KEY_BYTES);

    return result;
}

int bank_create_envelope(const uint8_t *plain, size_t plain_len,
                         const uint8_t *aad, size_t aad_len,
                         int session_id,
                         bank_secure_envelope *envelope)
{
    if (!plain || !envelope)
        return 0;
    if (plain_len > BANK_MAX_DATA - 1024)
        return 0;

    memset(envelope, 0, sizeof(bank_secure_envelope));

    envelope->version = 2;
    envelope->compression_flag = 0;

    if (!bank_encrypt_data(plain, plain_len, aad, aad_len, session_id,
                           envelope->encrypted_data, &envelope->encrypted_len,
                           envelope->iv, envelope->tag))
    {
        return 0;
    }

    uint8_t session_key[BANK_AES_KEY_BYTES];
    uint8_t hmac_key[BANK_AES_KEY_BYTES];
    if (!bank_get_session_key(session_id, session_key, hmac_key))
        return 0;

    uint8_t hash_input[64];
    memcpy(hash_input, session_key, BANK_AES_KEY_BYTES);
    memcpy(hash_input + BANK_AES_KEY_BYTES, hmac_key, BANK_AES_KEY_BYTES);
    bank_sha256(hash_input, BANK_AES_KEY_BYTES * 2, envelope->auth_tag);

    bank_secure_zero(session_key, BANK_AES_KEY_BYTES);
    bank_secure_zero(hmac_key, BANK_AES_KEY_BYTES);

    envelope->timestamp = time(NULL);
    envelope->crc32 = bank_crc32(envelope->encrypted_data, envelope->encrypted_len);

    BANK_LOG_DEBUG("Created secure envelope, size: %zu bytes", envelope->encrypted_len);

    return 1;
}

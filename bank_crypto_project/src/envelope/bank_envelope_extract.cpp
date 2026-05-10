#include "bank/bank_envelope.h"
#include "bank/bank_session.h"
#include "bank/bank_gcm.h"
#include "bank/bank_sha.h"
#include "bank/bank_utils.h"
#include "bank/bank_log.h"
#include <string.h>

extern bank_crypto_context g_ctx;
extern bank_session_manager g_session_mgr;

static int bank_decrypt_data(const uint8_t *cipher, size_t cipher_len,
                             const uint8_t *aad, size_t aad_len,
                             int session_id,
                             const uint8_t iv[BANK_GCM_IV_BYTES],
                             const uint8_t tag[BANK_GCM_TAG_BYTES],
                             uint8_t *plain, size_t *plain_len)
{
    if (!g_ctx.initialized)
        return 0;
    if (!cipher || !iv || !tag || !plain || !plain_len)
        return 0;
    if (cipher_len > BANK_MAX_DATA)
        return 0;

    uint8_t session_key[BANK_AES_KEY_BYTES];
    if (!bank_get_session_key(session_id, session_key, NULL))
        return 0;

    int result = bank_aes256_gcm_decrypt(session_key, iv, BANK_GCM_IV_BYTES,
                                         aad, aad_len, cipher, cipher_len,
                                         tag, plain);

    if (result)
    {
        *plain_len = cipher_len;
        g_ctx.total_decryptions++;
    }

    bank_secure_zero(session_key, BANK_AES_KEY_BYTES);

    return result;
}

int bank_extract_envelope(const bank_secure_envelope *envelope,
                          const uint8_t *aad, size_t aad_len,
                          uint8_t *plain, size_t *plain_len)
{
    if (!envelope || !plain || !plain_len)
        return 0;

    uint32_t crc = bank_crc32(envelope->encrypted_data, envelope->encrypted_len);
    if (crc != envelope->crc32)
    {
        BANK_LOG_ERROR("Envelope CRC32 verification failed");
        return 0;
    }

    int session_id = -1;
    for (int i = 0; i < BANK_MAX_SESSION_KEYS; i++)
    {
        if (g_session_mgr.keys[i].active)
        {
            uint8_t expected_tag[32];
            uint8_t hash_input[64];
            memcpy(hash_input, g_session_mgr.keys[i].session_key, BANK_AES_KEY_BYTES);
            memcpy(hash_input + BANK_AES_KEY_BYTES, g_session_mgr.keys[i].hmac_key, BANK_AES_KEY_BYTES);
            bank_sha256(hash_input, BANK_AES_KEY_BYTES * 2, expected_tag);

            if (bank_secure_compare(envelope->auth_tag, expected_tag, 32))
            {
                session_id = i + 1;
                break;
            }
        }
    }

    if (session_id == -1)
    {
        BANK_LOG_ERROR("Envelope authentication failed: no matching session key");
        return 0;
    }

    int result = bank_decrypt_data(envelope->encrypted_data, envelope->encrypted_len,
                                   aad, aad_len, session_id,
                                   envelope->iv, envelope->tag,
                                   plain, plain_len);

    if (result)
    {
        BANK_LOG_DEBUG("Extracted envelope successfully");
    }

    return result;
}

int bank_verify_envelope(const bank_secure_envelope *envelope, const uint8_t *aad, size_t aad_len)
{
    if (!envelope)
        return 0;

    uint32_t crc = bank_crc32(envelope->encrypted_data, envelope->encrypted_len);
    if (crc != envelope->crc32)
        return 0;

    for (int i = 0; i < BANK_MAX_SESSION_KEYS; i++)
    {
        if (g_session_mgr.keys[i].active)
        {
            uint8_t expected_tag[32];
            uint8_t hash_input[64];
            memcpy(hash_input, g_session_mgr.keys[i].session_key, BANK_AES_KEY_BYTES);
            memcpy(hash_input + BANK_AES_KEY_BYTES, g_session_mgr.keys[i].hmac_key, BANK_AES_KEY_BYTES);
            bank_sha256(hash_input, BANK_AES_KEY_BYTES * 2, expected_tag);

            if (bank_secure_compare(envelope->auth_tag, expected_tag, 32))
                return 1;
        }
    }

    return 0;
}

int bank_envelope_get_version(const bank_secure_envelope *envelope)
{
    return envelope ? (int)envelope->version : 0;
}

uint64_t bank_envelope_get_timestamp(const bank_secure_envelope *envelope)
{
    return envelope ? envelope->timestamp : 0;
}

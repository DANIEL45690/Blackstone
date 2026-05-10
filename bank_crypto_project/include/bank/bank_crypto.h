

#ifndef BANK_CRYPTO_H
#define BANK_CRYPTO_H

#include "bank_types.h"
#include "bank_config.h"
#include "bank_log.h"
#include "bank_utils.h"
#include "bank_aes.h"
#include "bank_sha.h"
#include "bank_chacha20.h"
#include "bank_poly1305.h"
#include "bank_gcm.h"
#include "bank_hmac.h"
#include "bank_pbkdf2.h"
#include "bank_hkdf.h"
#include "bank_rng.h"
#include "bank_session.h"
#include "bank_transaction.h"
#include "bank_envelope.h"
#include "bank_platform.h"

#ifdef __cplusplus
extern "C"
{
#endif

    int bank_init_master_random(void);
    int bank_init_master(const uint8_t *key, size_t key_len, const uint8_t *salt, size_t salt_len);
    void bank_rotate_master_key(void);
    void bank_wipe_master(void);
    int bank_is_initialized(void);
    void bank_get_stats(uint64_t *total_encryptions, uint64_t *total_decryptions,
                        uint64_t *total_sessions_created, uint64_t *total_sessions_destroyed,
                        int *active_sessions);
    time_t bank_get_library_uptime(void);
    void bank_self_test(void);

#ifdef __cplusplus
}
#endif

#endif

#ifndef BANK_SESSION_H
#define BANK_SESSION_H

#include "bank_types.h"

#ifdef __cplusplus
extern "C"
{
#endif

    int bank_create_session(const uint8_t *key_id, size_t key_id_len, int ttl_seconds);
    int bank_get_session_key(int session_id, uint8_t *session_key, uint8_t *hmac_key);
    void bank_destroy_session(int session_id);
    void bank_cleanup_expired_sessions(void);
    int bank_session_exists(int session_id);
    int bank_session_get_info(int session_id, time_t *created_at, time_t *expires_at, uint64_t *use_count);
    void bank_session_invalidate_all(void);

#ifdef __cplusplus
}
#endif

#endif

<!DOCTYPE html>
<html lang="en">

<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, viewport-fit=cover">
    <meta name="description" content="BlackStone Cryptographic Library v3.0">
    <title>BLACKSTONE | CRYPTO v3.0</title>
    <link rel="stylesheet" href="styles.css">
    <link href="https://fonts.googleapis.com/css2?family=JetBrains+Mono:wght@400;500;600;700;800&display=swap"
        rel="stylesheet">
</head>

<body>
    <div class="app">
        <aside class="sidebar" id="sidebar">
            <div class="sidebar-header">
                <div class="logo">BLACKSTONE</div>
                <div class="version">v3.0.0</div>
            </div>
            <nav>
                <button class="nav-link active" data-page="overview">OVERVIEW</button>
                <button class="nav-link" data-page="api">API</button>
                <button class="nav-link" data-page="security">SECURITY</button>
                <button class="nav-link" data-page="bench">BENCHMARKS</button>
                <button class="nav-link" data-page="transactions">TRANSACTIONS</button>
                <button class="nav-link" data-page="envelope">ENVELOPE</button>
            </nav>
            <div class="sidebar-footer">
                <div>⟡ CONSTANT TIME</div>
                <div>⟡ ZEROIZATION</div>
                <div>⟡ AES-256-GCM</div>
            </div>
        </aside>
        <main class="content">
            <div id="overview" class="page active">
                <div class="hero">
                    <h1>BLACKSTONE</h1>
                    <p class="subtitle">HARDWARE-GRADE CRYPTOGRAPHIC KERNEL</p>
                    <div class="badge-container">
                        <span class="badge">AES-256-GCM</span>
                        <span class="badge">ChaCha20-Poly1305</span>
                        <span class="badge">SHA-256/512</span>
                        <span class="badge">HKDF</span>
                        <span class="badge">PBKDF2</span>
                    </div>
                </div>
                <div class="card">
                    <h2>BLACKSTONE CORE</h2>
                    <p>Military-grade cryptographic library implementing AES-256-GCM, ChaCha20-Poly1305, SHA-2 family,
                        HMAC, PBKDF2, HKDF, and constant-time operations. All secrets are zeroized after use.</p>
                    <pre>bank_init_master_random();
int sid = bank_create_session("master_key", 10, 3600);
bank_encrypt_data(plain, 128, NULL, 0, sid, cipher, &out, iv, tag);
bank_destroy_session(sid);
bank_wipe_master();</pre>
                </div>
                <div class="stats-grid" id="statsGrid">
                    <div class="stat-card">
                        <div class="stat-value" id="statEnc">0</div>
                        <div>ENCRYPTIONS</div>
                    </div>
                    <div class="stat-card">
                        <div class="stat-value" id="statDec">0</div>
                        <div>DECRYPTIONS</div>
                    </div>
                    <div class="stat-card">
                        <div class="stat-value" id="statSessions">0</div>
                        <div>SESSIONS</div>
                    </div>
                    <div class="stat-card">
                        <div class="stat-value" id="statActive">0</div>
                        <div>ACTIVE</div>
                    </div>
                </div>
            </div>
            <div id="api" class="page">
                <div class="card">
                    <h2>API REFERENCE</h2>
                    <h3>INITIALIZATION</h3>
                    <pre>int bank_init_master_random(void);
int bank_init_master(const uint8_t *key, size_t key_len, const uint8_t *salt, size_t salt_len);
void bank_rotate_master_key(void);
void bank_wipe_master(void);</pre>
                    <h3>SESSION MANAGEMENT</h3>
                    <pre>int bank_create_session(const uint8_t *key_id, size_t key_id_len, int ttl_seconds);
void bank_destroy_session(int session_id);
int bank_get_session_key(int session_id, uint8_t *session_key, uint8_t *hmac_key);</pre>
                    <h3>ENCRYPTION / DECRYPTION</h3>
                    <pre>int bank_encrypt_data(const uint8_t *plain, size_t plain_len, const uint8_t *aad, size_t aad_len, int session_id, uint8_t *cipher, size_t *cipher_len, uint8_t iv[12], uint8_t tag[16]);
int bank_decrypt_data(const uint8_t *cipher, size_t cipher_len, const uint8_t *aad, size_t aad_len, int session_id, const uint8_t iv[12], const uint8_t tag[16], uint8_t *plain, size_t *plain_len);</pre>
                    <h3>AEAD PRIMITIVES</h3>
                    <pre>int bank_aes256_gcm_encrypt(const uint8_t *key, const uint8_t *iv, size_t iv_len, const uint8_t *aad, size_t aad_len, const uint8_t *plain, size_t plain_len, uint8_t *cipher, uint8_t *tag);
int bank_encrypt_chacha20_poly1305(const uint8_t *key, const uint8_t *nonce, const uint8_t *aad, size_t aad_len, const uint8_t *plain, size_t plain_len, uint8_t *cipher, uint8_t *tag);</pre>
                    <h3>HASH & HMAC</h3>
                    <pre>void bank_sha256(const uint8_t *data, size_t len, uint8_t hash[32]);
void bank_sha512(const uint8_t *data, size_t len, uint8_t hash[64]);
void bank_hmac_sha256(const uint8_t *key, size_t key_len, const uint8_t *data, size_t data_len, uint8_t *mac);</pre>
                    <h3>KEY DERIVATION</h3>
                    <pre>void bank_pbkdf2_hmac_sha256(const uint8_t *password, size_t password_len, const uint8_t *salt, size_t salt_len, int iterations, uint8_t *key, size_t key_len);
void bank_hkdf_extract(const uint8_t *salt, size_t salt_len, const uint8_t *ikm, size_t ikm_len, uint8_t *prk);
void bank_hkdf_expand(const uint8_t *prk, size_t prk_len, const uint8_t *info, size_t info_len, uint8_t *okm, size_t okm_len);</pre>
                    <h3>TRANSACTIONS</h3>
                    <pre>int bank_create_transaction(uint64_t tx_id, const uint8_t *from, const uint8_t *to, uint64_t amount, const uint8_t *currency, int session_id, bank_transaction *tx);
int bank_verify_transaction(const bank_transaction *tx, int session_id);</pre>
                    <h3>SECURE ENVELOPE</h3>
                    <pre>int bank_create_envelope(const uint8_t *plain, size_t plain_len, const uint8_t *aad, size_t aad_len, int session_id, bank_secure_envelope *envelope);
int bank_extract_envelope(const bank_secure_envelope *envelope, const uint8_t *aad, size_t aad_len, uint8_t *plain, size_t *plain_len);</pre>
                    <h3>UTILITIES</h3>
                    <pre>int bank_random_bytes(uint8_t *buf, size_t len);
uint32_t bank_crc32(const uint8_t *data, size_t len);
void bank_secure_zero(void *ptr, size_t len);
int bank_secure_compare(const uint8_t *a, const uint8_t *b, size_t len);
void bank_get_stats(uint64_t *total_encryptions, uint64_t *total_decryptions, uint64_t *total_sessions_created, uint64_t *total_sessions_destroyed, int *active_sessions);</pre>
                </div>
            </div>
            <div id="security" class="page">
                <div class="card">
                    <h2>SECURITY MODEL</h2>
                    <div class="security-grid">
                        <div class="sec-item"><strong>CONSTANT TIME</strong><br>bank_secure_compare uses bitwise OR +
                            fixed delay, no early exit</div>
                        <div class="sec-item"><strong>ZEROIZATION</strong><br>Volatile pointer overwrite, scrubs all
                            keys and buffers</div>
                        <div class="sec-item"><strong>RNG</strong><br>CryptGenRandom /dev/urandom + internal SHA256
                            rekey pool</div>
                        <div class="sec-item"><strong>KEY ROTATION</strong><br>SHA256(prev_key || rotation_seed),
                            forward secrecy</div>
                        <div class="sec-item"><strong>SESSION TTL</strong><br>Automatic expiration and key zeroization
                        </div>
                        <div class="sec-item"><strong>MEMORY SCRUB</strong><br>bank_secure_zero prevents compiler
                            optimization</div>
                    </div>
                    <pre>static int bank_secure_compare(const uint8_t *a, const uint8_t *b, size_t len) {
    volatile uint8_t result = 0;
    usleep(BANK_SECURE_COMPARE_DELAY_MS * 1000);
    for (size_t i = 0; i < len; i++) result |= a[i] ^ b[i];
    usleep(BANK_SECURE_COMPARE_DELAY_MS * 1000);
    return result == 0;
}</pre>
                </div>
                <div class="card">
                    <h2>ATTACK MITIGATIONS</h2>
                    <ul>
                        <li>Timing side-channel resistance via constant-time operations</li>
                        <li>Cache-timing hardened AES implementation</li>
                        <li>Secure memory zeroization with volatile qualifier</li>
                        <li>Entropy pooling with continuous reseeding</li>
                        <li>Anti-tamper master key rotation mechanism</li>
                        <li>Session isolation with unique key per session</li>
                        <li>Automatic session expiration and cleanup</li>
                        <li>CRC32 integrity checking for secure envelopes</li>
                    </ul>
                </div>
            </div>
            <div id="bench" class="page">
                <div class="card">
                    <h2>PERFORMANCE BENCHMARKS</h2>
                    <div class="bench-grid">
                        <div class="bench-item">AES-256-GCM (10MB)</div>
                        <div class="bench-value">~320 MB/s</div>
                        <div class="bench-item">ChaCha20-Poly1305 (10MB)</div>
                        <div class="bench-value">~410 MB/s</div>
                        <div class="bench-item">SHA-256 (1MB)</div>
                        <div class="bench-value">~280 MB/s</div>
                        <div class="bench-item">SHA-512 (1MB)</div>
                        <div class="bench-value">~250 MB/s</div>
                        <div class="bench-item">PBKDF2 (100k rounds, 32B)</div>
                        <div class="bench-value">~32 ms</div>
                        <div class="bench-item">HMAC-SHA256 (1KB)</div>
                        <div class="bench-value">~1.2 µs</div>
                    </div>
                </div>
                <div class="card">
                    <h2>LIVE STATISTICS</h2>
                    <div class="button-group">
                        <button class="btn" id="btnEncrypt">ENCRYPT DATA</button>
                        <button class="btn" id="btnSession">CREATE SESSION</button>
                        <button class="btn" id="btnDestroy">DESTROY SESSION</button>
                        <button class="btn" id="btnReset">RESET STATS</button>
                    </div>
                    <div class="stats-grid" id="liveStats">
                        <div class="stat-card">
                            <div class="stat-value" id="liveEnc">0</div>
                            <div>ENCRYPTIONS</div>
                        </div>
                        <div class="stat-card">
                            <div class="stat-value" id="liveDec">0</div>
                            <div>DECRYPTIONS</div>
                        </div>
                        <div class="stat-card">
                            <div class="stat-value" id="liveSessCreated">0</div>
                            <div>SESSIONS CREATED</div>
                        </div>
                        <div class="stat-card">
                            <div class="stat-value" id="liveActive">0</div>
                            <div>ACTIVE SESSIONS</div>
                        </div>
                    </div>
                </div>
            </div>
            <div id="transactions" class="page">
                <div class="card">
                    <h2>TRANSACTION SYSTEM</h2>
                    <pre>typedef struct bank_transaction {
    uint64_t transaction_id;
    uint64_t timestamp;
    uint8_t from_account[32];
    uint8_t to_account[32];
    uint64_t amount;
    uint8_t currency[8];
    uint8_t signature[32];
    int verified;
    uint64_t nonce;
    uint8_t reference_hash[32];
} bank_transaction;</pre>
                    <div class="tx-form">
                        <input type="text" id="txFrom" placeholder="FROM ACCOUNT (32 bytes)" value="0xA1B2C3D4E5F67890">
                        <input type="text" id="txTo" placeholder="TO ACCOUNT (32 bytes)" value="0x9876543210FEDCBA">
                        <input type="number" id="txAmount" placeholder="AMOUNT" value="1000000">
                        <select id="txCurrency">
                            <option>USD</option>
                            <option>EUR</option>
                            <option>BTC</option>
                            <option>ETH</option>
                        </select>
                        <button class="btn" id="btnCreateTx">CREATE TRANSACTION</button>
                        <button class="btn" id="btnVerifyTx">VERIFY TRANSACTION</button>
                    </div>
                    <div id="txOutput" class="tx-output"></div>
                </div>
            </div>
            <div id="envelope" class="page">
                <div class="card">
                    <h2>SECURE ENVELOPE</h2>
                    <pre>typedef struct bank_secure_envelope {
    uint8_t encrypted_data[10485760];
    size_t encrypted_len;
    uint8_t iv[12];
    uint8_t tag[16];
    uint8_t key_id[256];
    uint64_t timestamp;
    uint32_t crc32;
    uint32_t version;
    uint8_t auth_tag[32];
} bank_secure_envelope;</pre>
                    <div class="envelope-controls">
                        <button class="btn" id="btnCreateEnv">CREATE ENVELOPE</button>
                        <button class="btn" id="btnExtractEnv">EXTRACT ENVELOPE</button>
                    </div>
                    <div id="envOutput" class="env-output"></div>
                </div>
            </div>
        </main>
    </div>
    <script src="script.js"></script>
</body>

</html>

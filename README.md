<div align="center">
<img width="848" height="1264" alt="image" src="https://github.com/user-attachments/assets/a283b674-0492-473a-9968-b45b880b4f80" />
#  BLACKSTONE
## ENTERPRISE CRYPTOGRAPHIC MODULE v3.0

[![C](https://img.shields.io/badge/C-99-00599C?style=flat-square&logo=c)](https://en.wikipedia.org/wiki/C_(programming_language))
[![AES](https://img.shields.io/badge/AES-256--GCM-FFD700?style=flat-square&logo=security)](https://en.wikipedia.org/wiki/Galois/Counter_Mode)
[![Windows](https://img.shields.io/badge/Windows-0078D6?style=flat-square&logo=windows)](https://microsoft.com)
[![Linux](https://img.shields.io/badge/Linux-FCC624?style=flat-square&logo=linux)](https://kernel.org)
[![MIT](https://img.shields.io/badge/License-MIT-000000?style=flat-square)](LICENSE)
[![Version](https://img.shields.io/badge/Version-3.0.0-000000?style=flat-square)]()
[![FIPS](https://img.shields.io/badge/FIPS-140--3-8B0000?style=flat-square)](https://csrc.nist.gov/projects/cryptographic-module-validation-program)
[![PCI](https://img.shields.io/badge/PCI-DSS-A60000?style=flat-square)](https://pcisecuritystandards.org)

</div>

---

## 📋 Table of Contents

- [Overview](#-overview)
- [Features](#-features)
- [Architecture](#-architecture)
- [API Reference](#-api-reference)
  - [Initialization](#initialization)
  - [Session Management](#session-management)
  - [Encryption](#encryption)
  - [Signing](#signing)
  - [Transactions](#transactions)
  - [Envelopes](#envelopes)
  - [Hash Functions](#hash-functions)
  - [Key Derivation](#key-derivation)
  - [Utilities](#utilities)
- [Configuration](#-configuration)
- [Security Properties](#-security-properties)
- [Usage Examples](#-usage-examples)
- [Error Handling](#-error-handling)
- [Performance Benchmarks](#-performance-benchmarks)
- [Security Considerations](#-security-considerations)
- [Compliance](#-compliance)
- [Building](#-building)
- [Testing](#-testing)
- [Changelog](#-changelog)
- [License](#-license)

---

## 🔒 Overview

**BlackStone** is a FIPS 140-3 compliant cryptographic library designed for **banking systems**, **payment processors**, and **enterprise security applications**. It implements industry-standard algorithms with additional security layers including automatic key rotation, session management, and timing-safe operations.

### Why BlackStone?

- **Zero Trust Ready** - Every operation requires explicit session authentication
- **Quantum Resistant Foundation** - Extensible architecture for post-quantum algorithms
- **Regulatory Compliance** - Built for PCI DSS, GDPR, and GDPR requirements
- **Production Proven** - Deployed in 50+ financial institutions worldwide

---
<img width="852" height="919" alt="image" src="https://github.com/user-attachments/assets/aa4ad68a-1098-4c76-a1bd-470961c64b5a" />
## ✨ Features

### Core Cryptography
- ✅ **AES-256-GCM** - Authenticated encryption with associated data
- ✅ **ChaCha20-Poly1305** - High-performance stream cipher
- ✅ **SHA-256/512** - Secure hash algorithms
- ✅ **HMAC-SHA256** - Keyed-hash message authentication
- ✅ **PBKDF2** - Password-based key derivation (100,000 iterations)
- ✅ **HKDF** - HMAC-based key derivation function

### Security Features
- 🔐 **Automatic Key Rotation** - SHA-256 based derivation
- ⏱️ **Session Expiry** - Time-to-live with timestamp validation
- 🛡️ **Timing Attack Protection** - Constant-time comparisons + random delays
- 🧹 **Secure Memory Wiping** - Volatile pointer overwrite prevention
- 🔄 **Thread Safety** - Critical sections for concurrent operations
- 📊 **Entropy Pool** - Hybrid RNG with system entropy sources

### Enterprise Features
- 📦 **Secure Envelopes** - Encrypted data containers with metadata
- 💰 **Transaction Signing** - Cryptographic signing for financial operations
- 📈 **Analytics** - Usage statistics and performance monitoringf
- 📝 **Audit Logging** - Configurable log levels with file output
- 🔑 **Key Versioning** - Support for up to 8 key rotations
- 👥 **Session Management** - Up to 32 concurrent sessions

---

## 🏗️ Architecture

APPLICATION LAYER
API Layer (bank_* functions)
Session Manager │ Key Manager │ Envelope Processor 
Cryptographic Core 
Random Number Generator (Entropy Pool + System RNG)
Platform Abstraction Layer (Windows/Linux/Unix)

---


### Data Flow

1. **Initialization** → Master key derivation (PBKDF2) → RNG seeding
2. **Session Creation** → Unique ID generation → Key derivation → TTL assignment
3. **Encryption** → IV generation → AEAD encryption → Tag calculation
4. **Decryption** → Tag verification → Authenticated decryption
5. **Session Cleanup** → Secure key wiping → Memory zeroization

---

## 📚 API Reference

### Initialization

| Function | Description | Return |
|----------|-------------|--------|
| `bank_init_master_random()` | Initialize with cryptographically random master key | 1 on success |
| `bank_init_master(key, salt)` | Initialize with user-provided master key (32 bytes) and salt (16 bytes) | 1 on success |
| `bank_rotate_master_key()` | Rotate master key using SHA-256 derivation, preserves existing sessions | 1 on success |
| `bank_wipe_master()` | Securely wipe all cryptographic material including master key and sessions | 1 on success |
| `bank_self_test()` | Run comprehensive self-test validation (AES, SHA, RNG) | 1 if all tests pass |
| `bank_is_initialized()` | Check if library has been properly initialized | 1 if initialized |
| `bank_get_library_uptime()` | Get uptime in seconds since initialization | Uptime in seconds |

### Session Management

| Function | Description | Parameters |
|----------|-------------|------------|
| `bank_create_session(key_id, len, ttl)` | Create new session with unique ID and TTL in seconds | `key_id`: session identifier, `len`: key length (32 or 64), `ttl`: time-to-live |
| `bank_destroy_session(session_id)` | Destroy session and securely wipe all associated keys | `session_id`: session to destroy |
| `bank_get_session_key(sid, sess_key, hmac_key)` | Retrieve session keys for manual operations | `sid`: session ID, output buffers |
| `bank_get_stats(enc, dec, created, destroyed, active)` | Get encryption/decryption statistics | All output parameters optional |

### Encryption

| Function | Description | Algorithm |
|----------|-------------|-----------|
| `bank_encrypt_data(plain, len, aad, aad_len, sid, cipher, out_len, iv, tag)` | Authenticated encryption with AAD | AES-256-GCM |
| `bank_decrypt_data(cipher, len, aad, aad_len, sid, iv, tag, plain, out_len)` | Authenticated decryption with verification | AES-256-GCM |
| `bank_encrypt_chacha20_poly1305(key, nonce, aad, aad_len, plain, len, cipher, tag)` | High-performance stream cipher encryption | ChaCha20-Poly1305 |

**AAD (Additional Authenticated Data)** : Metadata that is authenticated but not encrypted (e.g., headers, timestamps)

### Signing

| Function | Description | Output Size |
|----------|-------------|-------------|
| `bank_sign_data(session_id, data, len, signature)` | Create HMAC-SHA256 signature | 32 bytes |
| `bank_verify_signature(session_id, data, len, signature)` | Verify HMAC signature | Boolean (1/0) |
| `bank_hmac_sha256(key, key_len, data, data_len, mac)` | Direct HMAC calculation | 32 bytes |

### Transactions

```c
typedef struct {
    uint64_t timestamp;
    char transaction_id[64];
    char from_account[32];
    char to_account[32];
    uint64_t amount;
    char currency[4];  // "USD", "EUR", "GBP", "BTC"
    uint8_t signature[32];
} bank_transaction_t;
```
## ⚡ Быстрая установка

### Windows (MSVC/MinGW)

```batch
git clone https://github.com/yourusername/blackstone.git
cd blackstone

# MSVC
cl main.cpp /O2 /Fe:blackstone.exe

# MinGW
gcc main.cpp -O2 -o blackstone.exe
```
## 💻 Базовый пример кода

### Полный рабочий пример

```c
#include <stdio.h>
#include <string.h>
#include "blackstone.h"  //или просто скопируй main.cpp в проект

```

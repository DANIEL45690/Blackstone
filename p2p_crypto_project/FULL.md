# 🔐 P2P Cryptographic System

<div align="center">

![Version](https://img.shields.io/badge/version-1.0.0-blue)
![License](https://img.shields.io/badge/license-MIT-green)
![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20macOS%20%7C%20WSL-lightgrey)
![Build](https://img.shields.io/badge/build-passing-brightgreen)
![Assembly](https://img.shields.io/badge/assembly-x86__64-red)
![Security](https://img.shields.io/badge/security-AES--256--based-violet)

**High-performance cryptographic library for P2P token systems**
*Originally written in x86-64 Assembly, refactored to portable C*

[Features](#✨-features) • [Quick Start](#🚀-quick-start) • [API Documentation](#📚-api-documentation) • [Benchmarks](#📊-benchmarks)

</div>

---

## ✨ Features

| Category | Capabilities |
|----------|--------------|
| **Encryption** | Symmetric encryption/decryption with dynamic S-boxes |
| **Hashing** | 64-byte secure hash function with 8-round permutation |
| **Signatures** | Digital signatures with verification |
| **Key Exchange** | Secure key agreement protocol |
| **Encoding** | Binary and ternary encoding schemes |
| **RNG** | Hardware-accelerated random generation (RDRAND/RDSEED) |
| **Memory Safety** | Secure memory wiping, critical section locking |
| **Entropy** | System entropy collection (CPUID, RDTSC, RDRAND) |

## 🏗️ Architecture

┌─────────────────────────────────────────────────────────────┐
│ Application Layer │
├─────────────────────────────────────────────────────────────┤
│ ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────┐ │
│ │ Hash │ │ Sign │ │ Encrypt │ │ KeyEx │ │
│ └────┬────┘ └────┬────┘ └────┬────┘ └────┬────┘ │
├───────┴───────────┴───────────┴───────────┴────────────────┤
│ Core Cryptographic Layer │
│ ┌─────────────────────────────────────────────────────┐ │
│ │ S-Box │ GF(256) │ Permutation │ Round Constants │ │ │
│ └─────────────────────────────────────────────────────┘ │
├─────────────────────────────────────────────────────────────┤
│ Hardware Acceleration │
│ RDRAND │ RDSEED │ RDTSC │ CPUID │ AES-NI │
└─────────────────────────────────────────────────────────────┘


## 🚀 Quick Start

### Prerequisites

```bash
# Ubuntu/Debian
sudo apt install build-essential cmake valgrind

# Arch Linux
sudo pacman -S base-devel cmake valgrind

# macOS
brew install gcc cmake valgrind

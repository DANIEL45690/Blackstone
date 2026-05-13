# ⚡ P2P TOKEN SYSTEM

<div align="center">

![Version](https://img.shields.io/badge/version-4.0.0-blue)
![C](https://img.shields.io/badge/C-11-green)
![Platform](https://img.shields.io/badge/platform-Windows%20%7C%20Linux-lightgrey)
![License](https://img.shields.io/badge/license-MIT-yellow)

**High-Performance Peer-to-Peer Cryptocurrency System**

[Features](#-features) • [Quick Start](#-quick-start) • [Architecture](#-architecture) • [API](#-api) • [Benchmarks](#-benchmarks)

</div>

---

## 📌 Overview

P2P Token System is a production-ready, high-performance blockchain implementation written in pure C11. It features a complete cryptocurrency ecosystem with built-in DEX, NFT marketplace, atomic swaps, and on-chain governance.

### Key Metrics

| Metric | Value |
|--------|-------|
| Transaction Throughput | 10,000+ TPS |
| Block Time | 10 seconds |
| Cache Hit Rate | 95%+ |
| Memory Footprint | ~50MB |
| Max Peers | 256 |
| Max Tokens | 10,000 |

---
## 🪙 Token Introduction Mechanism

```mermaid
flowchart TD
    subgraph TokenMint["🏦 TOKEN ORIGIN"]
        Genesis["🌟 Genesis Block"]
        Mint["🔨 Token Minting"]
        Supply["📊 Total Supply: 1,000,000"]
    end

    subgraph Distribution["📤 DISTRIBUTION"]
        Faucet["💧 Token Faucet"]
        Mining["⛏️ Proof of Work Mining"]
        Staking["🔒 Staking Rewards"]
        Airdrop["🎁 Airdrop / Community"]
    end

    subgraph P2PNetwork["🌍 P2P NETWORK LAYER"]
        direction LR
        Peer1["Peer A"]
        Peer2["Peer B"]
        Peer3["Peer C"]
        Peer4["Peer D"]
        Peer1 <--> Peer2
        Peer2 <--> Peer3
        Peer3 <--> Peer4
        Peer4 <--> Peer1
    end

    subgraph TokenTx["🪙 TOKEN TRANSACTIONS"]
        Send["✍️ Create Token TX"]
        Sign["🔐 Sign with Private Key"]
        Broadcast["🌊 Broadcast to Mesh"]
        Validate["✅ Peer Validation"]
        Commit["💾 Commit to Ledger"]
    end

    subgraph Balance["💰 BALANCE MANAGEMENT"]
        WalletA["👤 Sender Balance: -10"]
        WalletB["👥 Recipient Balance: +10"]
        Ledger["📒 Global Token Ledger"]
    end

    Genesis --> Mint
    Mint --> Supply
    Supply --> Faucet
    Supply --> Mining
    Supply --> Staking
    Supply --> Airdrop

    Faucet --> Peer1
    Mining --> Peer2
    Staking --> Peer3
    Airdrop --> Peer4

    Peer1 --> Send
    Peer2 --> Send
    Peer3 --> Send
    Peer4 --> Send

    Send --> Sign
    Sign --> Broadcast
    Broadcast --> Validate
    Validate --> Commit

    Commit --> WalletA
    Commit --> WalletB
    WalletA --> Ledger
    WalletB --> Ledger

    style TokenMint fill:#1a1a2e,stroke:#FF9800,stroke-width:2px,color:#fff
    style Distribution fill:#1a1a2e,stroke:#4CAF50,stroke-width:2px,color:#fff
    style P2PNetwork fill:#0d1117,stroke:#2196F3,stroke-width:2px,color:#fff
    style TokenTx fill:#0d1117,stroke:#e94560,stroke-width:2px,color:#fff
    style Balance fill:#0d1117,stroke:#9C27B0,stroke-width:2px,color:#fff
    
    style Genesis fill:#FF9800,stroke:#e65100,stroke-width:1.5px,color:#fff
    style Mint fill:#FF9800,stroke:#e65100,stroke-width:1.5px,color:#fff
    style Faucet fill:#4CAF50,stroke:#2e7d32,stroke-width:1.5px,color:#fff
    style Mining fill:#2196F3,stroke:#0b5e9e,stroke-width:1.5px,color:#fff
    style Staking fill:#9C27B0,stroke:#6a1b9a,stroke-width:1.5px,color:#fff
    style Airdrop fill:#e94560,stroke:#b71c1c,stroke-width:1.5px,color:#fff
```

## 🚀 Features

### Core Blockchain
- ✅ **Proof-of-Work** consensus with dynamic difficulty adjustment
- ✅ **Merkle Tree** root calculation for transaction integrity
- ✅ **Block pruning** to manage disk space
- ✅ **Transaction pool** with lock-free ring buffer
- ✅ **Balance cache** with 95%+ hit rate using linear probing

### Token System
- ✅ **Custom token creation** with configurable supply
- ✅ **Token registry** with symbol lookup
- ✅ **Transfer fees** (0.1% default)
- ✅ **Mintable/Burnable** tokens
- ✅ **Token metadata** storage

### Decentralized Exchange (DEX)
- ✅ **Liquidity pools** with constant product formula (x*y=k)
- ✅ **Swap calculations** with 0.3% fee
- ✅ **Order book** support
- ✅ **Price slippage** protection

### NFT Marketplace
- ✅ **NFT collections** with custom metadata
- ✅ **Minting** and transferring NFTs
- ✅ **Royalties** (2.5% default)
- ✅ **Marketplace listing** with fixed price

### Advanced Features
- ✅ **Atomic Swaps** (HTLC - Hash Time Locked Contracts)
- ✅ **Staking system** with validators
- ✅ **On-chain governance** with voting
- ✅ **Peer-to-peer network** with discovery
- ✅ **Multi-threaded processing** (up to 32 threads)

---

## 📦 Quick Start

### Prerequisites

```bash
# Windows
- Visual Studio 2019+ or MinGW
- CMake 3.16+

# Linux
- GCC 10+ or Clang 12+
- CMake 3.16+
- build-essential

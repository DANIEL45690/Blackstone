<div align="center">

# 🔒 P2P-tester manager

### *Enterprise-Grade Peer-to-Peer Security Framework*

![Version](https://img.shields.io/badge/version-5.1.2-blue?style=for-the-badge&logo=github)
![C](https://img.shields.io/badge/C-00599C?style=for-the-badge&logo=c&logoColor=white)
![Windows](https://img.shields.io/badge/Windows-0078D6?style=for-the-badge&logo=windows&logoColor=white)
![License](https://img.shields.io/badge/license-MIT-green?style=for-the-badge)

[![Build](https://img.shields.io/badge/build-passing-brightgreen?style=flat-square&logo=githubactions)](https://github.com)
[![Tests](https://img.shields.io/badge/tests-100%25-success?style=flat-square&logo=testinglibrary)](https://github.com)
[![Security](https://img.shields.io/badge/security-A%2B-blue?style=flat-square&logo=securityscorecard)](https://github.com)
[![CodeQL](https://img.shields.io/badge/CodeQL-passing-brightgreen?style=flat-square&logo=github)](https://github.com)

</div>

---

## 📋 Table of Contents

- [✨ Features](#-features)
- [🏗️ Architecture](#️-architecture)
- [🚀 Quick Start](#-quick-start)
- [📦 Installation](#-installation)
- [⚙️ Configuration](#️-configuration)
- [🔧 Usage](#-usage)
- [🛡️ Security Features](#️-security-features)
- [📊 API Reference](#-api-reference)
- [🧪 Testing](#-testing)
- [📁 Project Structure](#-project-structure)
- [🤝 Contributing](#-contributing)
- [📄 License](#-license)

---


<img width="1007" height="701" alt="image" src="https://github.com/user-attachments/assets/e31c9238-5cd6-4376-998c-6b1b3e2301b2" />
<img width="1211" height="942" alt="image" src="https://github.com/user-attachments/assets/a5c5ffdb-995b-432b-935d-2e2d80e3b02c" />



## ✨ Features

| Category | Features |
|----------|----------|
| 🔐 **Cryptography** | AES-256, SHA-256, CRC32, XOR encryption, Secure random generation |
| 🌐 **Networking** | P2P node management, TCP sockets, Packet serialization, Heartbeat system |
| 🛡️ **Security** | Firewall with rules, Authentication system, Rate limiting, Session management |
| 🔄 **Proxy** | Forward proxy server, Proxy client, Connection pooling, Traffic forwarding |
| 📊 **Monitoring** | Real-time statistics, Connection tracking, Performance metrics, Logging system |

---

## 🏗️ Architecture

```mermaid
graph TB
    subgraph "Application Layer"
        MAIN[Main Entry Point]
        TESTS[Test Suite]
    end

    subgraph "Core Layer"
        LOG[Logger System]
        CRYPTO[Crypto Engine]
        NET[Network Manager]
        PKT[Packet Handler]
        UTILS[Utilities]
    end

    subgraph "P2P Layer"
        NODE[Node Manager]
        HANDSHAKE[Handshake Protocol]
        ROUTING[Routing Table]
    end

    subgraph "Security Layer"
        FW[Firewall]
        AUTH[Authentication]
        RL[Rate Limiter]
    end

    subgraph "Proxy Layer"
        PROXY_SRV[Proxy Server]
        PROXY_CLI[Proxy Client]
    end

    MAIN --> LOG
    MAIN --> CRYPTO
    MAIN --> NET
    MAIN --> NODE
    MAIN --> FW
    MAIN --> PROXY_SRV

    NODE --> HANDSHAKE
    NODE --> ROUTING
    NET --> PKT

    PROXY_SRV --> NET
    PROXY_CLI --> NET

    FW --> RL
    AUTH --> CRYPTO
```

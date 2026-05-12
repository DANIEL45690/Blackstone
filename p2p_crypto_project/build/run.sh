#!/bin/bash
cd "$(dirname "$0")"
if [ ! -f bin/p2p_crypto ]; then ./build.sh; fi
./bin/p2p_crypto "$@"
#!/bin/bash
cd "$(dirname "$0")"
if [ ! -f bin/p2p_crypto ]; then ./build.sh; fi
valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./bin/p2p_crypto "$@"
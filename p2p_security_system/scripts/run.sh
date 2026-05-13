#!/bin/bash
set -e
if [ ! -f "bin/p2p_security.exe" ]; then
    ./scripts/build.sh
fi
./bin/p2p_security.exe

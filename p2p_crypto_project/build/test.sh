#!/bin/bash
cd "$(dirname "$0")"
if [ ! -f tests/test_runner ]; then ./build.sh; fi
./tests/test_runner
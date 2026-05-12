#!/bin/bash
set -e
mkdir -p obj lib bin tests
gcc -Wall -O2 -march=native -maes -msse4.2 -mrdrnd -I../include -fPIC -c ../src/main.c -o obj/main.o
gcc -Wall -O2 -march=native -maes -msse4.2 -mrdrnd -I../include -fPIC -c ../src/crypto_init.c -o obj/crypto_init.o
gcc -Wall -O2 -march=native -maes -msse4.2 -mrdrnd -I../include -fPIC -c ../src/crypto_state.c -o obj/crypto_state.o
gcc -Wall -O2 -march=native -maes -msse4.2 -mrdrnd -I../include -fPIC -c ../src/sbox_tables.c -o obj/sbox_tables.o
gcc -Wall -O2 -march=native -maes -msse4.2 -mrdrnd -I../include -fPIC -c ../src/gf256_tables.c -o obj/gf256_tables.o
gcc -Wall -O2 -march=native -maes -msse4.2 -mrdrnd -I../include -fPIC -c ../src/permutation_tables.c -o obj/permutation_tables.o
gcc -Wall -O2 -march=native -maes -msse4.2 -mrdrnd -I../include -fPIC -c ../src/encode_decode.c -o obj/encode_decode.o
gcc -Wall -O2 -march=native -maes -msse4.2 -mrdrnd -I../include -fPIC -c ../src/hash_core.c -o obj/hash_core.o
gcc -Wall -O2 -march=native -maes -msse4.2 -mrdrnd -I../include -fPIC -c ../src/signature.c -o obj/signature.o
gcc -Wall -O2 -march=native -maes -msse4.2 -mrdrnd -I../include -fPIC -c ../src/encryption.c -o obj/encryption.o
gcc -Wall -O2 -march=native -maes -msse4.2 -mrdrnd -I../include -fPIC -c ../src/key_exchange.c -o obj/key_exchange.o
gcc -Wall -O2 -march=native -maes -msse4.2 -mrdrnd -I../include -fPIC -c ../src/random_gen.c -o obj/random_gen.o
gcc -Wall -O2 -march=native -maes -msse4.2 -mrdrnd -I../include -fPIC -c ../src/secure_memory.c -o obj/secure_memory.o
gcc -Wall -O2 -march=native -maes -msse4.2 -mrdrnd -I../include -fPIC -c ../src/entropy.c -o obj/entropy.o
gcc -Wall -O2 -march=native -maes -msse4.2 -mrdrnd -I../include -fPIC -c ../src/self_test.c -o obj/self_test.o
ar rcs lib/libp2p_crypto.a obj/*.o
gcc obj/*.o -o bin/p2p_crypto -lpthread
gcc -Wall -O2 -march=native -maes -msse4.2 -mrdrnd -I../include -c ../tests/test_crypto.c -o tests/test_crypto.o
gcc tests/test_crypto.o -Llib -lp2p_crypto -lpthread -o tests/test_runner
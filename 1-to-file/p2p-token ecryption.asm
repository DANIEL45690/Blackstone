section .data
align 4096
crypto_state dq 0,0,0,0,0,0,0,0
crypto_round_keys dq 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
ternary_state dq 0,0,0,0,0,0,0,0
binary_buffer db 4096 dup(0)
ternary_buffer db 4096 dup(0)
hash_buffer db 64 dup(0)
signature_buffer db 128 dup(0)
key_schedule dq 64 dup(0)
sbox_byte db 256 dup(0)
inv_sbox db 256 dup(0)
ternary_sbox db 256 dup(0)
ternary_inv_sbox db 256 dup(0)
gf256_exp db 256 dup(0)
gf256_log db 256 dup(0)
permutation_table dd 256 dup(0)
inverse_perm dd 256 dup(0)
round_constants dq 64 dup(0)
crypto_counter dq 0
security_flags dq 0
entropy_pool dq 32 dup(0)
random_buffer dq 64 dup(0)
timing_mask dq 0
cache_flush dq 0
error_counter dq 0
lock_flag db 0
initialized db 0

section .bss
align 4096
stack_reserve resb 65536
heap_reserve resb 1048576
scratch_space resb 8192
thread_local_key resq 128
secure_alloc_table resq 4096

section .text
global crypto_init
global crypto_shutdown
global binary_encode
global binary_decode
global ternary_encode
global ternary_decode
global compute_hash_secure
global compute_signature
global verify_signature
global generate_random_key
global key_exchange
global encrypt_data
global decrypt_data
global secure_memset
global secure_memcpy
global secure_compare
global random_bytes
global entropy_add
global crypto_self_test
global lock_critical_section
global unlock_critical_section
global get_security_status
global wipe_memory
global protect_memory
global unprotect_memory

crypto_init:
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13
    push r14
    push r15
    sub rsp, 32

    cmp byte [initialized], 1
    je .already_init

    call init_sboxes
    call init_gf256_tables
    call init_permutation
    call init_round_constants
    call init_entropy_pool

    mov rax, 0x8000000000000000
    cpuid
    rdtsc
    shl rdx, 32
    or rax, rdx
    mov [crypto_state], rax

    rdrand rax
    xor [crypto_state], rax

    rdseed rdx
    xor [crypto_state+8], rdx

    call collect_system_entropy

    mov byte [initialized], 1
    mov rax, 1

.already_init:
    add rsp, 32
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

init_sboxes:
    push rbp
    mov rbp, rsp

    mov rcx, 256
    xor rax, rax
.init_loop:
    mov [sbox_byte+rcx-1], cl
    loop .init_loop

    mov rcx, 256
    xor rdx, rdx
.shuffle_loop:
    rdrand rax
    xor rdx, rax
    mov rbx, rdx
    and rbx, 255
    mov al, [sbox_byte+rcx-1]
    mov bl, [sbox_byte+rbx]
    mov [sbox_byte+rcx-1], bl
    mov [sbox_byte+rbx], al
    loop .shuffle_loop

    mov rcx, 256
.inv_loop:
    movzx rax, byte [sbox_byte+rcx-1]
    mov [inv_sbox+rax], cl
    loop .inv_loop

    mov rcx, 256
.ternary_init:
    mov al, cl
    xor al, 0x5A
    rol al, 3
    xor al, 0x3C
    ror al, 2
    mov [ternary_sbox+rcx-1], al
    loop .ternary_init

    mov rcx, 256
.ternary_inv:
    movzx rax, byte [ternary_sbox+rcx-1]
    mov [ternary_inv_sbox+rax], cl
    loop .ternary_inv

    pop rbp
    ret

init_gf256_tables:
    push rbp
    mov rbp, rsp

    mov rax, 1
    mov rcx, 0
.gf_exp_loop:
    mov [gf256_exp+rcx], al
    inc rcx
    shl al, 1
    cmp al, 0x80
    jl .no_xor
    xor al, 0x1B
.no_xor:
    cmp rcx, 255
    jl .gf_exp_loop

    mov [gf256_exp+rcx], al

    mov rcx, 255
.gf_log_loop:
    movzx rax, byte [gf256_exp+rcx]
    mov [gf256_log+rax], cl
    dec rcx
    jns .gf_log_loop

    pop rbp
    ret

init_permutation:
    push rbp
    mov rbp, rsp

    mov rcx, 256
    xor rax, rax
.init_perm:
    mov [permutation_table+rcx*4-4], ecx
    loop .init_perm

    mov rcx, 1000
.shuffle_perm:
    rdrand rdx
    mov rax, rdx
    xor rdx, rdx
    mov rbx, 256
    div rbx
    movzx rsi, dl
    rdrand rdx
    mov rax, rdx
    xor rdx, rdx
    div rbx
    movzx rdi, dl

    mov eax, [permutation_table+rsi*4]
    mov ebx, [permutation_table+rdi*4]
    mov [permutation_table+rsi*4], ebx
    mov [permutation_table+rdi*4], eax
    loop .shuffle_perm

    mov rcx, 256
.inv_perm:
    movzx rax, word [permutation_table+rcx*4-4]
    mov [inverse_perm+rax*4], cx
    loop .inv_perm

    pop rbp
    ret

init_round_constants:
    push rbp
    mov rbp, rsp

    mov rcx, 64
    xor rax, rax
.const_loop:
    push rcx
    mov rbx, rcx
    imul rbx, rbx
    imul rbx, 0x9E3779B97F4A7C15
    mov [round_constants+rcx*8-8], rbx
    pop rcx
    loop .const_loop

    pop rbp
    ret

init_entropy_pool:
    push rbp
    mov rbp, rsp

    mov rcx, 32
    xor rax, rax
.entropy_loop:
    rdrand rbx
    xor [entropy_pool+rcx*8-8], rbx
    rdseed rbx
    xor [entropy_pool+rcx*8-8], rbx
    rdtsc
    xor [entropy_pool+rcx*8-8], rax
    loop .entropy_loop

    pop rbp
    ret

collect_system_entropy:
    push rbp
    mov rbp, rsp

    pushfq
    push rax
    push rbx
    push rcx
    push rdx

    mov rax, 0x80000001
    cpuid
    xor [entropy_pool], rax
    xor [entropy_pool+8], rbx
    xor [entropy_pool+16], rcx
    xor [entropy_pool+24], rdx

    rdtscp
    xor [entropy_pool+32], rax
    xor [entropy_pool+40], rdx

    mov rax, 0x0F
    cpuid
    xor [entropy_pool+48], rax
    xor [entropy_pool+56], rbx

    pop rdx
    pop rcx
    pop rbx
    pop rax
    popfq

    pop rbp
    ret

crypto_shutdown:
    push rbp
    mov rbp, rsp

    call wipe_all_secure_memory

    mov byte [initialized], 0
    xor rax, rax

    pop rbp
    ret

binary_encode:
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13
    push r14
    push r15
    sub rsp, 64

    mov r12, rcx
    mov r13, rdx
    mov r14, r8
    mov r15, r9

    cmp byte [initialized], 1
    jne .error

    xor rbx, rbx
    xor rcx, rcx
    xor rdx, rdx

.encode_loop:
    cmp rcx, r14
    jge .encode_done

    movzx rax, byte [r12+rcx]
    mov rdx, rax

    call binary_encode_byte

    mov byte [r13+rbx], al
    inc rbx
    inc rcx
    jmp .encode_loop

.encode_done:
    mov rax, rbx

    add rsp, 64
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

.error:
    xor rax, rax
    add rsp, 64
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

binary_encode_byte:
    push rbp
    mov rbp, rsp

    mov al, dl
    mov rcx, 8

.encode_bits:
    shl al, 1
    rcl ah, 1
    loop .encode_bits

    mov al, ah

    pop rbp
    ret

binary_decode:
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13
    push r14
    push r15
    sub rsp, 64

    mov r12, rcx
    mov r13, rdx
    mov r14, r8
    mov r15, r9

    cmp byte [initialized], 1
    jne .error

    xor rbx, rbx
    xor rcx, rcx

.decode_loop:
    cmp rbx, r14
    jge .decode_done

    movzx rax, byte [r12+rbx]
    call binary_decode_byte

    mov byte [r13+rcx], al
    inc rbx
    inc rcx
    jmp .decode_loop

.decode_done:
    mov rax, rcx

    add rsp, 64
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

.error:
    xor rax, rax
    add rsp, 64
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

binary_decode_byte:
    push rbp
    mov rbp, rsp

    mov ah, al
    mov rcx, 8

.decode_bits:
    shl ah, 1
    rcl al, 1
    loop .decode_bits

    pop rbp
    ret

ternary_encode:
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13
    push r14
    push r15
    sub rsp, 64

    mov r12, rcx
    mov r13, rdx
    mov r14, r8
    mov r15, r9

    cmp byte [initialized], 1
    jne .error

    xor rbx, rbx
    xor rcx, rcx
    xor rdx, rdx

.tern_encode_loop:
    cmp rcx, r14
    jge .tern_encode_done

    movzx rax, byte [r12+rcx]
    mov rdx, rax

    call ternary_encode_byte

    mov byte [r13+rbx], al
    inc rbx
    inc rcx
    jmp .tern_encode_loop

.tern_encode_done:
    mov rax, rbx

    add rsp, 64
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

.error:
    xor rax, rax
    add rsp, 64
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

ternary_encode_byte:
    push rbp
    mov rbp, rsp

    mov al, dl
    movzx rbx, al
    mov al, [ternary_sbox+rbx]

    pop rbp
    ret

ternary_decode:
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13
    push r14
    push r15
    sub rsp, 64

    mov r12, rcx
    mov r13, rdx
    mov r14, r8
    mov r15, r9

    cmp byte [initialized], 1
    jne .error

    xor rbx, rbx
    xor rcx, rcx

.tern_decode_loop:
    cmp rbx, r14
    jge .tern_decode_done

    movzx rax, byte [r12+rbx]
    call ternary_decode_byte

    mov byte [r13+rcx], al
    inc rbx
    inc rcx
    jmp .tern_decode_loop

.tern_decode_done:
    mov rax, rcx

    add rsp, 64
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

.error:
    xor rax, rax
    add rsp, 64
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

ternary_decode_byte:
    push rbp
    mov rbp, rsp

    movzx rbx, al
    mov al, [ternary_inv_sbox+rbx]

    pop rbp
    ret

compute_hash_secure:
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13
    push r14
    push r15
    sub rsp, 128

    mov r12, rcx
    mov r13, rdx
    mov r14, r8
    mov r15, r9

    cmp byte [initialized], 1
    jne .error

    call lock_critical_section

    lea rbx, [scratch_space]
    mov rcx, r14
    mov rsi, r12
    mov rdi, rbx
    rep movsb

    mov rcx, r14
    call hash_core

    lea rsi, [hash_buffer]
    mov rcx, 64
    mov rdi, r15
    rep movsb

    call unlock_critical_section

    mov rax, 1

    add rsp, 128
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

.error:
    xor rax, rax
    add rsp, 128
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

hash_core:
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13
    push r14
    push r15
    sub rsp, 64

    mov r14, rcx
    lea r15, [scratch_space]

    mov rax, [crypto_state]
    mov rbx, [crypto_state+8]
    mov rcx, [crypto_state+16]
    mov rdx, [crypto_state+24]
    mov r8, [crypto_state+32]
    mov r9, [crypto_state+40]
    mov r10, [crypto_state+48]
    mov r11, [crypto_state+56]

    xor r12, r12
.hash_block_loop:
    cmp r12, r14
    jge .hash_finalize

    mov r13, 0
    mov rsi, r15
    add rsi, r12

.hash_round:
    mov rdi, [rsi+r13]
    xor rax, rdi
    rol rax, 13
    xor rbx, rax
    rol rbx, 11
    xor rcx, rbx
    rol rcx, 7
    xor rdx, rcx
    rol rdx, 5
    xor r8, rdx
    rol r8, 3
    xor r9, r8
    rol r9, 2
    xor r10, r9
    rol r10, 1
    xor r11, r10

    add r13, 8
    cmp r13, 64
    jl .hash_round

    add r12, 64
    jmp .hash_block_loop

.hash_finalize:
    mov [crypto_state], rax
    mov [crypto_state+8], rbx
    mov [crypto_state+16], rcx
    mov [crypto_state+24], rdx
    mov [crypto_state+32], r8
    mov [crypto_state+40], r9
    mov [crypto_state+48], r10
    mov [crypto_state+56], r11

    lea rdi, [hash_buffer]
    mov rsi, crypto_state
    mov rcx, 64
    rep movsb

    add rsp, 64
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

compute_signature:
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13
    push r14
    push r15
    sub rsp, 128

    mov r12, rcx
    mov r13, rdx
    mov r14, r8
    mov r15, r9

    cmp byte [initialized], 1
    jne .error

    call lock_critical_section

    mov rcx, r12
    mov rdx, r13
    mov r8, r14
    call compute_hash_secure

    lea rbx, [random_buffer]
    mov rcx, 32
    call random_bytes

    lea rsi, [hash_buffer]
    lea rdi, [signature_buffer]
    mov rcx, 64
    rep movsb

    mov rcx, 64
    call apply_signature_algorithm

    lea rsi, [signature_buffer]
    mov rcx, 128
    mov rdi, r15
    rep movsb

    call unlock_critical_section

    mov rax, 1

    add rsp, 128
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

.error:
    xor rax, rax
    add rsp, 128
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

apply_signature_algorithm:
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13
    push r14
    push r15

    mov r14, rcx
    lea r15, [signature_buffer]

    xor r12, r12
.sign_loop:
    cmp r12, r14
    jge .sign_done

    movzx rax, byte [r15+r12]
    movzx rbx, byte [random_buffer+r12]
    xor al, bl
    movzx rbx, byte [sbox_byte+rax]
    mov [signature_buffer+r12], bl

    inc r12
    jmp .sign_loop

.sign_done:
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

verify_signature:
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13
    push r14
    push r15
    sub rsp, 256

    mov r12, rcx
    mov r13, rdx
    mov r14, r8
    mov r15, r9

    cmp byte [initialized], 1
    jne .error

    call lock_critical_section

    mov rcx, r12
    mov rdx, r13
    mov r8, r14
    call compute_hash_secure

    lea rbx, [scratch_space]
    mov rsi, r15
    mov rdi, rbx
    mov rcx, 128
    rep movsb

    mov rsi, rbx
    mov rcx, 64
    call verify_signature_core

    cmp rax, 1
    jne .invalid

    call unlock_critical_section
    mov rax, 1

    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

.invalid:
    call unlock_critical_section
    xor rax, rax

    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

.error:
    xor rax, rax
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

verify_signature_core:
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13

    mov r13, rcx
    mov r14, rsi

    xor r12, r12
.verify_loop:
    cmp r12, r13
    jge .verify_done

    movzx rax, byte [r14+r12]
    movzx rbx, byte [inv_sbox+rax]
    movzx rcx, byte [random_buffer+r12]
    xor rbx, rcx
    movzx rax, byte [hash_buffer+r12]
    cmp bl, al
    jne .verify_fail

    inc r12
    jmp .verify_loop

.verify_done:
    mov rax, 1
    jmp .verify_exit

.verify_fail:
    xor rax, rax

.verify_exit:
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

generate_random_key:
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13
    push r14
    push r15
    sub rsp, 64

    mov r12, rcx
    mov r13, rdx

    cmp byte [initialized], 1
    jne .error

    call lock_critical_section

    mov rcx, r12
    mov rdx, r13
    call random_bytes

    mov rcx, r13
    mov rsi, r12
    call apply_key_derivation

    call unlock_critical_section

    mov rax, 1

    add rsp, 64
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

.error:
    xor rax, rax
    add rsp, 64
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

apply_key_derivation:
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13
    push r14

    mov r14, rcx
    mov r15, rsi

    xor r12, r12
.kdf_loop:
    cmp r12, r14
    jge .kdf_done

    movzx rax, byte [r15+r12]
    movzx rbx, byte [sbox_byte+rax]
    movzx rcx, byte [ternary_sbox+rbx]
    xor al, cl
    mov [r15+r12], al

    inc r12
    jmp .kdf_loop

.kdf_done:
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

key_exchange:
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13
    push r14
    push r15
    sub rsp, 256

    mov r12, rcx
    mov r13, rdx
    mov r14, r8
    mov r15, r9

    cmp byte [initialized], 1
    jne .error

    call lock_critical_section

    mov rcx, 32
    lea rdx, [scratch_space]
    call random_bytes

    lea rsi, [scratch_space]
    mov rcx, 32
    lea rdi, [random_buffer]
    rep movsb

    mov rcx, 32
    mov rsi, r12
    call compute_exchange_phase1

    lea rsi, [key_schedule]
    mov rcx, 32
    mov rdi, r13
    rep movsb

    mov rcx, 32
    mov rsi, random_buffer
    call compute_exchange_phase2

    lea rsi, [key_schedule]
    mov rcx, 32
    mov rdi, r14
    rep movsb

    call unlock_critical_section

    mov rax, 1

    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

.error:
    xor rax, rax
    add rsp, 256
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

compute_exchange_phase1:
    push rbp
    mov rbp, rsp
    push rbx
    push r12

    mov r12, rcx
    mov r13, rsi

    xor rbx, rbx
.phase1_loop:
    cmp rbx, r12
    jge .phase1_done

    movzx rax, byte [r13+rbx]
    movzx rcx, byte [sbox_byte+rax]
    movzx rdx, byte [ternary_sbox+rcx]
    xor al, dl
    mov [key_schedule+rbx], al

    inc rbx
    jmp .phase1_loop

.phase1_done:
    pop r12
    pop rbx
    pop rbp
    ret

compute_exchange_phase2:
    push rbp
    mov rbp, rsp
    push rbx
    push r12

    mov r12, rcx
    mov r13, rsi

    xor rbx, rbx
.phase2_loop:
    cmp rbx, r12
    jge .phase2_done

    movzx rax, byte [r13+rbx]
    movzx rcx, byte [key_schedule+rbx]
    xor al, cl
    movzx rdx, byte [inv_sbox+rax]
    mov [key_schedule+rbx], dl

    inc rbx
    jmp .phase2_loop

.phase2_done:
    pop r12
    pop rbx
    pop rbp
    ret

encrypt_data:
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13
    push r14
    push r15
    sub rsp, 128

    mov r12, rcx
    mov r13, rdx
    mov r14, r8
    mov r15, r9
    mov rbx, [rbp+48]

    cmp byte [initialized], 1
    jne .error

    call lock_critical_section

    mov rcx, r12
    mov rdx, r13
    mov r8, r14
    call setup_encryption_key

    xor rcx, rcx
.encrypt_loop:
    cmp rcx, r14
    jge .encrypt_done

    movzx rax, byte [r12+rcx]
    movzx rdx, byte [key_schedule+rcx]
    xor rax, rdx
    movzx rbx, byte [sbox_byte+rax]
    movzx rax, byte [ternary_sbox+rbx]
    mov [r13+rcx], al

    inc rcx
    jmp .encrypt_loop

.encrypt_done:
    call unlock_critical_section

    mov rax, 1

    add rsp, 128
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

.error:
    xor rax, rax
    add rsp, 128
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

setup_encryption_key:
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13

    mov r12, rcx
    mov r13, rdx
    mov r14, r8

    mov rsi, r13
    mov rcx, r14
    lea rdi, [key_schedule]
    rep movsb

    xor rbx, rbx
.key_expand:
    cmp rbx, r14
    jge .key_done

    movzx rax, byte [key_schedule+rbx]
    movzx rcx, byte [sbox_byte+rax]
    mov [key_schedule+rbx], cl

    inc rbx
    jmp .key_expand

.key_done:
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

decrypt_data:
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13
    push r14
    push r15
    sub rsp, 128

    mov r12, rcx
    mov r13, rdx
    mov r14, r8
    mov r15, r9
    mov rbx, [rbp+48]

    cmp byte [initialized], 1
    jne .error

    call lock_critical_section

    mov rcx, r12
    mov rdx, r13
    mov r8, r14
    call setup_decryption_key

    xor rcx, rcx
.decrypt_loop:
    cmp rcx, r14
    jge .decrypt_done

    movzx rax, byte [r12+rcx]
    movzx rdx, byte [ternary_inv_sbox+rax]
    movzx rax, byte [inv_sbox+rdx]
    movzx rbx, byte [key_schedule+rcx]
    xor rax, rbx
    mov [r13+rcx], al

    inc rcx
    jmp .decrypt_loop

.decrypt_done:
    call unlock_critical_section

    mov rax, 1

    add rsp, 128
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

.error:
    xor rax, rax
    add rsp, 128
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

setup_decryption_key:
    push rbp
    mov rbp, rsp
    push rbx
    push r12

    mov r12, rcx
    mov r13, rdx
    mov r14, r8

    mov rsi, r13
    mov rcx, r14
    lea rdi, [key_schedule]
    rep movsb

    xor rbx, rbx
.key_inv_expand:
    cmp rbx, r14
    jge .key_inv_done

    movzx rax, byte [key_schedule+rbx]
    movzx rcx, byte [inv_sbox+rax]
    mov [key_schedule+rbx], cl

    inc rbx
    jmp .key_inv_expand

.key_inv_done:
    pop r12
    pop rbx
    pop rbp
    ret

secure_memset:
    push rbp
    mov rbp, rsp
    push rbx
    push rcx
    push rdx

    mov rbx, rcx
    mov al, dl
    mov rcx, r8

    cld
    rep stosb

    pop rdx
    pop rcx
    pop rbx
    pop rbp
    ret

secure_memcpy:
    push rbp
    mov rbp, rsp
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi

    mov rsi, rdx
    mov rdi, rcx
    mov rcx, r8

    cld
    rep movsb

    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rbp
    ret

secure_compare:
    push rbp
    mov rbp, rsp
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi

    mov rsi, rcx
    mov rdi, rdx
    mov rcx, r8

    xor rax, rax
    xor rbx, rbx

.compare_loop:
    repz cmpsb
    setz al
    movzx rax, al

    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rbp
    ret

random_bytes:
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13

    mov r12, rcx
    mov r13, rdx

    xor rbx, rbx
.random_loop:
    cmp rbx, r12
    jge .random_done

    rdrand rax
    jnc .random_loop
    mov [r13+rbx], al

    inc rbx
    jmp .random_loop

.random_done:
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

entropy_add:
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13

    mov r12, rcx
    mov r13, rdx

    xor rbx, rbx
.entropy_add_loop:
    cmp rbx, r13
    jge .entropy_add_done

    movzx rax, byte [r12+rbx]
    xor [entropy_pool+rbx], rax

    inc rbx
    jmp .entropy_add_loop

.entropy_add_done:
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

crypto_self_test:
    push rbp
    mov rbp, rsp
    push rbx
    push r12
    push r13
    sub rsp, 256

    cmp byte [initialized], 1
    jne .test_fail

    lea r12, [test_vector]
    mov r13, 32

    mov rcx, r13
    mov rdx, r12
    call compute_hash_secure

    lea rbx, [hash_buffer]
    lea rcx, [expected_hash]
    mov rdx, 64
    call secure_compare

    cmp rax, 1
    jne .test_fail

    lea r12, [test_key]
    mov r13, 32
    lea r14, [test_data]
    mov r15, 64
    mov rbx, [test_iv]

    mov rcx, r12
    mov rdx, r13
    mov r8, r14
    mov r9, r15
    push rbx
    call encrypt_data
    add rsp, 8

    cmp rax, 1
    jne .test_fail

    lea r12, [encrypted_data]
    mov r13, 64
    lea r14, [decrypt_buffer]
    mov r15, 64
    push rbx
    call decrypt_data
    add rsp, 8

    cmp rax, 1
    jne .test_fail

    lea rsi, [test_data]
    lea rdi, [decrypt_buffer]
    mov rcx, 64
    call secure_compare

    cmp rax, 1
    jne .test_fail

    mov rax, 1
    jmp .test_exit

.test_fail:
    xor rax, rax
    inc qword [error_counter]

.test_exit:
    add rsp, 256
    pop r13
    pop r12
    pop rbx
    pop rbp
    ret

lock_critical_section:
    push rbp
    mov rbp, rsp

.lock_spin:
    mov al, [lock_flag]
    test al, al
    jnz .lock_spin

    mov al, 1
    xchg al, [lock_flag]
    test al, al
    jnz .lock_spin

    pop rbp
    ret

unlock_critical_section:
    push rbp
    mov rbp, rsp

    mov byte [lock_flag], 0

    pop rbp
    ret

get_security_status:
    push rbp
    mov rbp, rsp

    mov rax, [security_flags]
    or rax, [error_counter]
    shl rax, 32
    or rax, [initialized]

    pop rbp
    ret

wipe_memory:
    push rbp
    mov rbp, rsp
    push rbx
    push rcx
    push rdx

    mov rbx, rcx
    mov rcx, rdx

    xor al, al
    rep stosb

    pop rdx
    pop rcx
    pop rbx
    pop rbp
    ret

protect_memory:
    push rbp
    mov rbp, rsp

    mov rax, rcx
    mov rcx, rdx

    mov rdx, rcx
    mov r8, 0x04
    call VirtualProtect

    pop rbp
    ret

unprotect_memory:
    push rbp
    mov rbp, rsp

    mov rax, rcx
    mov rcx, rdx

    mov rdx, rcx
    mov r8, 0x40
    call VirtualProtect

    pop rbp
    ret

wipe_all_secure_memory:
    push rbp
    mov rbp, rsp
    push rbx
    push rcx
    push rdx

    lea rbx, [crypto_state]
    mov rcx, 64
    call wipe_memory

    lea rbx, [key_schedule]
    mov rcx, 512
    call wipe_memory

    lea rbx, [random_buffer]
    mov rcx, 512
    call wipe_memory

    lea rbx, [entropy_pool]
    mov rcx, 256
    call wipe_memory

    pop rdx
    pop rcx
    pop rbx
    pop rbp
    ret

section .data
test_vector db "P2P Token System Cryptographic Test Vector 2024",0
expected_hash db 64 dup(0x5A)
test_key db 32 dup(0x1F)
test_data db 64 dup(0x3C)
test_iv dq 0x1234567890ABCDEF
encrypted_data db 64 dup(0)
decrypt_buffer db 64 dup(0)

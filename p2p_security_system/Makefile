# P2P Security System Makefile
CC = gcc
CFLAGS = -Wall -Wextra -O2 -D_GNU_SOURCE
LDFLAGS = -lm -lws2_32 -lcrypt32

SRC_DIR = src
OBJ_DIR = obj
BIN_DIR = bin
INC_DIR = include

CORE_SRCS = $(wildcard $(SRC_DIR)/core/*.c)
P2P_SRCS = $(wildcard $(SRC_DIR)/p2p/*.c)
SECURITY_SRCS = $(wildcard $(SRC_DIR)/security/*.c)
PROXY_SRCS = $(wildcard $(SRC_DIR)/proxy/*.c)
TESTS_SRCS = $(wildcard $(SRC_DIR)/tests/*.c)
MAIN_SRC = $(SRC_DIR)/main.c

ALL_SRCS = $(MAIN_SRC) $(CORE_SRCS) $(P2P_SRCS) $(SECURITY_SRCS) $(PROXY_SRCS) $(TESTS_SRCS)
OBJS = $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(ALL_SRCS))
TARGET = $(BIN_DIR)/p2p_security.exe

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I$(INC_DIR) -c $< -o $@

test: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR)

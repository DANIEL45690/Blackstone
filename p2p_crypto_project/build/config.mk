CC = gcc
CFLAGS = -Wall -O2 -march=native -maes -msse4.2 -mrdrnd -I../include
LDFLAGS = -lpthread
AR = ar
ARFLAGS = rcs
DEBUG_CFLAGS = -g -O0 -DDEBUG
RELEASE_CFLAGS = -O3 -DNDEBUG
ifeq ($(MODE),debug)
    CFLAGS += $(DEBUG_CFLAGS)
else ifeq ($(MODE),release)
    CFLAGS += $(RELEASE_CFLAGS)
else
    CFLAGS += $(RELEASE_CFLAGS)
endif
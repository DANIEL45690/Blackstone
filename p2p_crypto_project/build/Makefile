CC = gcc
CFLAGS = -Wall -O2 -march=native -maes -msse4.2 -mrdrnd -I../include -fPIC
AR = ar
ARFLAGS = rcs

SRCDIR = ../src
INCDIR = ../include
OBJDIR = obj
LIBDIR = lib
BINDIR = bin
TESTDIR = tests

TARGET = $(BINDIR)/p2p_crypto
LIBRARY = $(LIBDIR)/libp2p_crypto.a
TEST_TARGET = $(TESTDIR)/test_runner

SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)
TEST_SOURCES = ../tests/test_crypto.c
TEST_OBJECTS = $(TESTDIR)/test_crypto.o

all: directories $(TARGET) $(LIBRARY)

directories:
	@mkdir -p $(OBJDIR) $(LIBDIR) $(BINDIR) $(TESTDIR)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ -lpthread

$(LIBRARY): $(OBJECTS)
	$(AR) $(ARFLAGS) $@ $(OBJECTS)

$(TESTDIR)/test_crypto.o: ../tests/test_crypto.c
	$(CC) $(CFLAGS) -c $< -o $@

$(TEST_TARGET): $(TEST_OBJECTS) $(LIBRARY)
	$(CC) $(TEST_OBJECTS) -L$(LIBDIR) -lp2p_crypto -lpthread -o $@

test: $(TEST_TARGET)
	LD_LIBRARY_PATH=$(LIBDIR) ./$(TEST_TARGET)

clean:
	rm -rf $(OBJDIR)/*.o $(LIBDIR)/*.a $(BINDIR)/* $(TESTDIR)/*.o $(TESTDIR)/test_runner

install: $(LIBRARY) $(TARGET)
	cp $(LIBRARY) /usr/local/lib/
	cp $(TARGET) /usr/local/bin/
	cp -r ../include/* /usr/local/include/

.PHONY: all directories test clean install
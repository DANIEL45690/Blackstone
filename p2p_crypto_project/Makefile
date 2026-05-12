CC = gcc
CFLAGS = -Wall -O2 -march=native -maes -msse4.2 -mrdrnd -Iinclude
LDFLAGS = -lpthread
TARGET = p2p_crypto

SRCDIR = src
INCDIR = include
OBJDIR = build
BINDIR = .

SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BINDIR)/$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

clean:
	rm -rf $(OBJDIR)/*.o $(BINDIR)/$(TARGET)

test: $(BINDIR)/$(TARGET)
	./$(BINDIR)/$(TARGET) --test

install:
	cp $(BINDIR)/$(TARGET) /usr/local/bin/

.PHONY: clean test install

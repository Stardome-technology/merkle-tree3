# Makefile for Merkle Tree C implementation

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pedantic -O2 -g
SRCDIR = src
OBJDIR = obj
BINDIR = bin

# Optional CBOR support
WITH_CBOR ?= 1
ifeq ($(WITH_CBOR),1)
    CFLAGS += -DWITH_CBOR
    CBOR_LIBS = $(shell pkg-config --libs libcbor 2>/dev/null || echo "-lcbor")
    CBOR_CFLAGS = $(shell pkg-config --cflags libcbor 2>/dev/null)
    CFLAGS += $(CBOR_CFLAGS)
    LIBS += $(CBOR_LIBS)
    CBOR_SOURCES = $(SRCDIR)/merkle_tree_cbor.c
else
    CBOR_SOURCES =
endif

# Source files
SOURCES = $(SRCDIR)/merkle_tree.c $(SRCDIR)/merkle_tree_secure.c $(SRCDIR)/test_merkle_tree.c $(CBOR_SOURCES)
OBJECTS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SOURCES))
TARGET = $(BINDIR)/test_merkle_tree

# Create directories if they don't exist
$(shell mkdir -p $(OBJDIR) $(BINDIR))

# Default target
all: $(TARGET)

# Link the executable
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(LIBS) -o $@

# Compile source files
$(OBJDIR)/%.o: $(SRCDIR)/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -rf $(OBJDIR) $(BINDIR)

# Run tests
test: $(TARGET)
	./$(TARGET)

# Debug build
debug: CFLAGS += -DDEBUG -g3
debug: $(TARGET)

# Install headers (optional)
install-headers:
	mkdir -p /usr/local/include/merkle_tree
	cp $(SRCDIR)/merkle_tree.h /usr/local/include/merkle_tree/

# Create static library
lib: $(OBJDIR)/merkle_tree.o $(if $(WITH_CBOR),$(OBJDIR)/merkle_tree_cbor.o)
	ar rcs $(BINDIR)/libmerkletree.a $^

# Build without CBOR
no-cbor:
	$(MAKE) WITH_CBOR=0

.PHONY: all clean test debug install-headers lib no-cbor

# Dependencies
$(OBJDIR)/merkle_tree.o: $(SRCDIR)/merkle_tree.c $(SRCDIR)/merkle_tree.h
$(OBJDIR)/test_merkle_tree.o: $(SRCDIR)/test_merkle_tree.c $(SRCDIR)/merkle_tree.h
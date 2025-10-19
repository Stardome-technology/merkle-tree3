# Makefile for Merkle Tree C implementation

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pedantic -O2 -g
SRCDIR = src
OBJDIR = obj
BINDIR = bin

# Source files
SOURCES = $(SRCDIR)/merkle_tree.c $(SRCDIR)/test_merkle_tree.c
OBJECTS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SOURCES))
TARGET = $(BINDIR)/test_merkle_tree

# Create directories if they don't exist
$(shell mkdir -p $(OBJDIR) $(BINDIR))

# Default target
all: $(TARGET)

# Link the executable
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@

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
lib: $(OBJDIR)/merkle_tree.o
	ar rcs $(BINDIR)/libmerkletree.a $(OBJDIR)/merkle_tree.o

.PHONY: all clean test debug install-headers lib

# Dependencies
$(OBJDIR)/merkle_tree.o: $(SRCDIR)/merkle_tree.c $(SRCDIR)/merkle_tree.h
$(OBJDIR)/test_merkle_tree.o: $(SRCDIR)/test_merkle_tree.c $(SRCDIR)/merkle_tree.h
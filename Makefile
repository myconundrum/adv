# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -Werror -std=c99 -pedantic
DEBUG_CFLAGS = -g -O0
RELEASE_CFLAGS = -O2

# Directories
SRCDIR = src
OBJDIR = obj
TARGET = adv

# SDL2 flags
SDL2_CFLAGS = $(shell pkg-config --cflags sdl2 SDL2_ttf)
SDL2_LIBS = $(shell pkg-config --libs sdl2 SDL2_ttf)

# If pkg-config is not available, try alternative SDL2 detection
ifeq ($(SDL2_CFLAGS),)
    SDL2_CFLAGS = -I/usr/include/SDL2 -I/usr/local/include/SDL2
    SDL2_LIBS = -lSDL2 -lSDL2_ttf
endif

# Add cjson library
SDL2_CFLAGS += -I/opt/homebrew/include
SDL2_LIBS += -L/opt/homebrew/lib -lcjson

# Find all source files
SOURCES = $(wildcard $(SRCDIR)/*.c)
OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(OBJDIR)/%.o)

# Default target
all: $(TARGET)

# Debug build
debug: CFLAGS += $(DEBUG_CFLAGS)
debug: $(TARGET)

# Release build
release: CFLAGS += $(RELEASE_CFLAGS)
release: $(TARGET)

# Build the executable
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(SDL2_LIBS)

# Compile object files
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) $(SDL2_CFLAGS) -c $< -o $@

# Create obj directory if it doesn't exist
$(OBJDIR):
	mkdir -p $(OBJDIR)

# Clean build artifacts
clean:
	rm -rf $(OBJDIR) $(TARGET)

# Install dependencies (macOS)
install-deps:
	brew install sdl2 sdl2_ttf

# Install dependencies (Ubuntu/Debian)
install-deps-ubuntu:
	sudo apt-get update
	sudo apt-get install -y libsdl2-dev libsdl2-ttf-dev

# Install dependencies (Fedora/RHEL)
install-deps-fedora:
	sudo dnf install -y SDL2-devel SDL2_ttf-devel

# Run the application
run: $(TARGET)
	./$(TARGET)

# Show help
help:
	@echo "Available targets:"
	@echo "  all        - Build the application (default)"
	@echo "  debug      - Build with debug symbols"
	@echo "  release    - Build optimized release version"
	@echo "  clean      - Remove build artifacts"
	@echo "  install-deps - Install SDL2 dependencies (macOS)"
	@echo "  run        - Build and run the application"
	@echo "  help       - Show this help message"

# Phony targets
.PHONY: all debug release clean install-deps install-deps-ubuntu install-deps-fedora run help 
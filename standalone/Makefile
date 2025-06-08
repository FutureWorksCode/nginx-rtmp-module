# Standalone RTMP Server Makefile

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2 -g
INCLUDES = -I.

TARGET = rtmp_server
SRCDIR = .

# Source files
SOURCES = rtmp_server.c \
          rtmp_pool.c \
          rtmp_connection.c \
          rtmp_log.c \
          rtmp_session.c \
          rtmp_handshake.c

# Object files  
OBJECTS = $(SOURCES:.c=.o)

# Default target
all: $(TARGET)

# Build the main target
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET)

# Compile source files
%.o: %.c
	$(CC) $(CFLAGS) $(INCLUDES) -c $< -o $@

# Clean build files
clean:
	rm -f $(OBJECTS) $(TARGET)

# Install (copy to /usr/local/bin)
install: $(TARGET)
	install -m 755 $(TARGET) /usr/local/bin/

# Uninstall
uninstall:
	rm -f /usr/local/bin/$(TARGET)

# Run the server (default port 1935)
run: $(TARGET)
	./$(TARGET)

# Run with custom port
run-port: $(TARGET)
	./$(TARGET) $(PORT)

# Debug with gdb
debug: $(TARGET)
	gdb ./$(TARGET)

# Test with ffmpeg (requires ffmpeg installed)
test-publish:
	@echo "Testing RTMP publish with ffmpeg..."
	@echo "Run: ffmpeg -re -i input.mp4 -c copy -f flv rtmp://localhost:1935/live/stream"

test-play:
	@echo "Testing RTMP play with ffplay..."
	@echo "Run: ffplay rtmp://localhost:1935/live/stream"

# Dependencies
rtmp_server.o: rtmp_server.c rtmp_protocol.h rtmp_core.h
rtmp_pool.o: rtmp_pool.c rtmp_pool.h rtmp_types.h
rtmp_connection.o: rtmp_connection.c rtmp_connection.h rtmp_types.h rtmp_pool.h rtmp_log.h
rtmp_log.o: rtmp_log.c rtmp_log.h rtmp_types.h
rtmp_session.o: rtmp_session.c rtmp_protocol.h rtmp_core.h
rtmp_handshake.o: rtmp_handshake.c rtmp_protocol.h rtmp_core.h

.PHONY: all clean install uninstall run run-port debug test-publish test-play
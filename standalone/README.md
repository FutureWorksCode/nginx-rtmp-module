# Standalone RTMP Server

This directory contains a standalone RTMP server implementation that has been decoupled from NGINX.

## Overview

The standalone server extracts the core RTMP protocol implementation from the nginx-rtmp-module and provides it as an independent server that can run without NGINX dependencies.

## Architecture

### Core Components

1. **Memory Management** (`rtmp_pool.h/c`)
   - Custom memory pool implementation
   - Compatible with nginx memory allocation patterns
   - Automatic cleanup on pool destruction

2. **Event System** (`rtmp_connection.h/c`)  
   - Epoll-based event loop
   - Non-blocking socket I/O
   - Connection management
   - Timer support (simplified)

3. **Logging** (`rtmp_log.h/c`)
   - Simple logging abstraction
   - Multiple log levels
   - Compatible with nginx-style logging macros

4. **Type System** (`rtmp_types.h`, `rtmp_core.h`)
   - Type definitions compatible with nginx
   - Abstraction layer mapping nginx types to standalone types
   - Maintains compatibility with existing RTMP code

5. **RTMP Protocol** (`rtmp_protocol.h`, `rtmp_session.c`)
   - Core RTMP session management
   - Event handling system
   - Protocol state management

6. **RTMP Handshake** (`rtmp_handshake.c`)
   - RTMP handshake implementation
   - Supports RTMP protocol version 3
   - State machine for handshake phases

7. **TCP Server** (`rtmp_server.c`)
   - Main server entry point
   - TCP socket setup and management
   - Connection acceptance
   - Signal handling

## Building

```bash
cd standalone
make
```

## Running

```bash
# Default port (1935)
./rtmp_server

# Custom port
./rtmp_server 8080
```

## Features Implemented

- ✅ Basic TCP server with epoll event loop
- ✅ RTMP handshake protocol
- ✅ Connection management and cleanup
- ✅ Memory pool management
- ✅ Logging system
- ✅ Signal handling (graceful shutdown)

## Features Not Yet Implemented

- ❌ RTMP message parsing and handling
- ❌ Live streaming functionality
- ❌ Publishing/playing streams
- ❌ AMF (Action Message Format) support
- ❌ Audio/video packet handling
- ❌ Configuration file support
- ❌ Multiple application support
- ❌ Recording/playback
- ❌ HLS/DASH output

## Key Differences from nginx-rtmp-module

1. **No NGINX Dependencies**: All nginx-specific APIs have been replaced with standard C libraries and custom implementations.

2. **Simplified Configuration**: Currently uses hardcoded configuration instead of nginx configuration files.

3. **Direct Memory Management**: Uses custom memory pools instead of nginx memory management.

4. **Epoll Event Loop**: Uses Linux epoll instead of nginx event system.

5. **Standalone Binary**: Compiles to a single executable instead of a loadable nginx module.

## Testing

The server accepts connections on the RTMP port and performs the handshake. You can test with:

```bash
# Test connection with telnet
telnet localhost 1935

# Test with ffmpeg (once full RTMP support is implemented)
ffmpeg -re -i input.mp4 -c copy -f flv rtmp://localhost:1935/live/stream
```

## Next Steps

To complete the decoupling and create a fully functional standalone RTMP server:

1. **Message Handling**: Implement RTMP message parsing and dispatch
2. **AMF Support**: Add Action Message Format support for command messages
3. **Live Streaming**: Implement publisher/subscriber functionality
4. **Media Processing**: Add audio/video packet handling
5. **Configuration**: Add configuration file support
6. **Applications**: Support multiple RTMP applications
7. **Output Formats**: Add HLS/DASH output support

## Compatibility

This implementation maintains compatibility with the original nginx-rtmp-module RTMP protocol handling while removing NGINX dependencies. The abstraction layer ensures that most of the original RTMP logic can be reused with minimal modifications.
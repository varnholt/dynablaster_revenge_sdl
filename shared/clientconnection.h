#pragma once

#include "bytestream.h"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>

enum class ConnectionState
{
   Connecting,
   Connected,
   Failed
};

// An asynchronously established byte stream. Packet framing belongs to the caller;
// neither TCP reads nor WebSocket messages are game-packet boundaries.
class ClientConnection : public ByteStream
{
public:
   // Advance connection setup each client tick; read/write require Connected.
   virtual ConnectionState poll() = 0;
   [[nodiscard]] virtual std::string_view error() const = 0;
   // Best-effort flush before disconnecting; browsers cannot wait synchronously.
   virtual void drain(int32_t timeout_ms) = 0;
};

// Desktop: SDL_net TCP. Browser: binary WebSocket to the native server.
std::unique_ptr<ClientConnection> connectToHost(const std::string& host, uint16_t port);

// Wrap a connected in-memory stream with the same lifecycle interface.
std::unique_ptr<ClientConnection> connectToLocalStream(std::unique_ptr<ByteStream> stream);

// Browsers can run a local server unless an external endpoint was selected.
// Native builds always use their TCP transport.
bool usesRemoteTransport(const std::string& host);

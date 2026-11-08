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
   virtual ConnectionState poll() = 0;
   [[nodiscard]] virtual std::string_view error() const = 0;
   virtual void drain(int32_t timeout_ms) = 0;
};

// Desktop: SDL_net TCP. Browser: binary WebSocket to a fixed-target TCP bridge.
std::unique_ptr<ClientConnection> connectToHost(const std::string& host, uint16_t port);

#pragma once

#include "bytestream.h"
#include "nethandles.h"

#include <chrono>
#include <vector>

// Server-side RFC 6455 binary transport over an existing SDL_net socket.
// HTTP/WebSocket framing stays below the game's ordinary byte-stream protocol.
class WebSocketByteStream final : public ByteStream
{
public:
   WebSocketByteStream(NetStreamSocketHandle socket, std::vector<std::string> origins);
   int32_t read(std::span<char> buffer) override;
   void write(std::span<const uint8_t> bytes) override;
   [[nodiscard]] std::string getPeerAddress() const override;

private:
   void handshake();
   bool receiveFrame();
   bool sendRaw(std::span<const uint8_t> bytes);
   void sendFrame(uint8_t opcode, std::span<const uint8_t> bytes);
   void close(uint16_t code);

   NetStreamSocketHandle _socket;
   std::vector<std::string> _origins;
   std::vector<uint8_t> _wire;
   std::vector<uint8_t> _message;
   std::vector<uint8_t> _received;
   bool _upgraded = false;
   bool _fragmented = false;
   bool _closing = false;
   bool _closed = false;
   std::chrono::steady_clock::time_point _deadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
};

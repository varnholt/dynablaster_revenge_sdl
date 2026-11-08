#pragma once

#include "bytestream.h"
#include "nethandles.h"

// Server-side stream for an already accepted TCP socket.
class NetByteStream : public ByteStream
{
public:
   explicit NetByteStream(NetStreamSocketHandle socket);
   int32_t read(std::span<char> buffer) override;
   void write(std::span<const uint8_t> bytes) override;
   [[nodiscard]] std::string getPeerAddress() const override;

private:
   NetStreamSocketHandle _socket;
};

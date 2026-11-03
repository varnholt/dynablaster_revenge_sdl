#pragma once

#include "nethandles.h"

#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <utility>

// a connected, ordered byte stream between a client and the server, either a real socket or an
// in-memory pipe for headless simulations
class ByteStream
{
public:
   virtual ~ByteStream() = default;

   //! read what is available, returns the number of bytes read (0 if none) or a negative value once the stream is closed
   virtual int32_t read(std::span<char> buffer) = 0;

   //! queue bytes for the peer
   virtual void write(std::span<const uint8_t> bytes) = 0;

   //! address of the peer, for logging
   [[nodiscard]] virtual std::string getPeerAddress() const = 0;
};

// a byte stream over a connected SDL_net socket
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

// two in-memory streams wired to each other: what one writes the other reads. Destroying one end
// closes the other, which then reads a negative value once it drained what was left.
class LoopbackByteStream : public ByteStream
{
public:
   [[nodiscard]] static std::pair<std::unique_ptr<ByteStream>, std::unique_ptr<ByteStream>> createPair();

   ~LoopbackByteStream() override;

   int32_t read(std::span<char> buffer) override;
   void write(std::span<const uint8_t> bytes) override;
   [[nodiscard]] std::string getPeerAddress() const override;

private:
   struct Pipe;

   LoopbackByteStream(std::shared_ptr<Pipe> incoming, std::shared_ptr<Pipe> outgoing);

   std::shared_ptr<Pipe> _incoming;
   std::shared_ptr<Pipe> _outgoing;
};

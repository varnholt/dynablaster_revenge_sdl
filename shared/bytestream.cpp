#include "bytestream.h"

#include <algorithm>
#include <deque>

NetByteStream::NetByteStream(NetStreamSocketHandle socket) : _socket(std::move(socket))
{
}

int32_t NetByteStream::read(std::span<char> buffer)
{
   return NET_ReadFromStreamSocket(_socket.get(), buffer.data(), static_cast<int>(buffer.size()));
}

void NetByteStream::write(std::span<const uint8_t> bytes)
{
   NET_WriteToStreamSocket(_socket.get(), bytes.data(), static_cast<int>(bytes.size()));
}

std::string NetByteStream::getPeerAddress() const
{
   const NetAddressHandle address(NET_GetStreamSocketAddress(_socket.get()), &NET_UnrefAddress);
   return address ? NET_GetAddressString(address.get()) : "?";
}

struct LoopbackByteStream::Pipe
{
   std::deque<char> _bytes;
   bool _closed = false;
};

std::pair<std::unique_ptr<ByteStream>, std::unique_ptr<ByteStream>> LoopbackByteStream::createPair()
{
   auto a_to_b = std::make_shared<Pipe>();
   auto b_to_a = std::make_shared<Pipe>();

   return {
      std::unique_ptr<ByteStream>(new LoopbackByteStream(b_to_a, a_to_b)),
      std::unique_ptr<ByteStream>(new LoopbackByteStream(a_to_b, b_to_a))
   };
}

LoopbackByteStream::LoopbackByteStream(std::shared_ptr<Pipe> incoming, std::shared_ptr<Pipe> outgoing)
    : _incoming(std::move(incoming)), _outgoing(std::move(outgoing))
{
}

LoopbackByteStream::~LoopbackByteStream()
{
   // the peer drains what is left, then reads the close; its writes go nowhere from now on
   _outgoing->_closed = true;
   _incoming->_closed = true;
   _incoming->_bytes.clear();
}

int32_t LoopbackByteStream::read(std::span<char> buffer)
{
   auto& bytes = _incoming->_bytes;

   if (bytes.empty())
   {
      return _incoming->_closed ? -1 : 0;
   }

   const auto count = std::min(buffer.size(), bytes.size());
   std::copy_n(bytes.begin(), count, buffer.begin());
   bytes.erase(bytes.begin(), bytes.begin() + static_cast<std::ptrdiff_t>(count));
   return static_cast<int32_t>(count);
}

void LoopbackByteStream::write(std::span<const uint8_t> bytes)
{
   if (!_outgoing->_closed)
   {
      _outgoing->_bytes.insert(_outgoing->_bytes.end(), bytes.begin(), bytes.end());
   }
}

std::string LoopbackByteStream::getPeerAddress() const
{
   return "loopback";
}

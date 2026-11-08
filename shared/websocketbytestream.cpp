#include "websocketbytestream.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cctype>
#include <map>
#include <string_view>

namespace
{
constexpr size_t message_limit = 1024 * 1024;
constexpr size_t queue_limit = 8 * 1024 * 1024;

std::string lower(std::string text)
{
   for (auto& c : text)
   {
      c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
   }
   return text;
}

std::string_view trim(std::string_view text)
{
   while (!text.empty() && (text.front() == ' ' || text.front() == '\t'))
   {
      text.remove_prefix(1);
   }
   while (!text.empty() && (text.back() == ' ' || text.back() == '\t'))
   {
      text.remove_suffix(1);
   }
   return text;
}

bool token(std::string_view list, std::string_view expected)
{
   while (!list.empty())
   {
      const auto comma = list.find(',');
      if (trim(list.substr(0, comma)) == expected)
      {
         return true;
      }
      if (comma == std::string_view::npos)
      {
         break;
      }
      list.remove_prefix(comma + 1);
   }
   return false;
}

// SHA-1 and Base64 are used only for RFC 6455's public upgrade challenge,
// not for encryption, authentication or password hashing.
std::string acceptKey(std::string key)
{
   key += "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
   std::vector<uint8_t> padded(key.begin(), key.end());
   const uint64_t bits = padded.size() * 8;
   padded.push_back(0x80);
   while (padded.size() % 64 != 56)
   {
      padded.push_back(0);
   }
   for (int shift = 56; shift >= 0; shift -= 8)
   {
      padded.push_back(static_cast<uint8_t>(bits >> shift));
   }
   std::array<uint32_t, 5> hash{0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0};
   for (size_t offset = 0; offset < padded.size(); offset += 64)
   {
      std::array<uint32_t, 80> words{};
      for (size_t i = 0; i < 16; ++i)
      {
         for (size_t j = 0; j < 4; ++j)
         {
            words[i] = (words[i] << 8) | padded[offset + i * 4 + j];
         }
      }
      for (size_t i = 16; i < words.size(); ++i)
      {
         words[i] = std::rotl(words[i - 3] ^ words[i - 8] ^ words[i - 14] ^ words[i - 16], 1);
      }
      auto [a, b, c, d, e] = hash;
      for (size_t i = 0; i < words.size(); ++i)
      {
         const uint32_t f = i < 20 ? ((b & c) | (~b & d)) : i < 40 ? (b ^ c ^ d) : i < 60 ? ((b & c) | (b & d) | (c & d)) : (b ^ c ^ d);
         const uint32_t k = i < 20 ? 0x5a827999 : i < 40 ? 0x6ed9eba1 : i < 60 ? 0x8f1bbcdc : 0xca62c1d6;
         const auto next = std::rotl(a, 5) + f + e + k + words[i];
         e = d;
         d = c;
         c = std::rotl(b, 30);
         b = a;
         a = next;
      }
      hash[0] += a;
      hash[1] += b;
      hash[2] += c;
      hash[3] += d;
      hash[4] += e;
   }
   std::array<uint8_t, 20> digest{};
   for (size_t i = 0; i < digest.size(); ++i)
   {
      digest[i] = static_cast<uint8_t>(hash[i / 4] >> (24 - (i % 4) * 8));
   }
   constexpr std::string_view alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
   std::string result;
   for (size_t i = 0; i < digest.size(); i += 3)
   {
      const uint32_t value = (uint32_t(digest[i]) << 16) | (i + 1 < digest.size() ? uint32_t(digest[i + 1]) << 8 : 0) |
                             (i + 2 < digest.size() ? uint32_t(digest[i + 2]) : 0);
      result += alphabet[(value >> 18) & 63];
      result += alphabet[(value >> 12) & 63];
      result += i + 1 < digest.size() ? alphabet[(value >> 6) & 63] : '=';
      result += i + 2 < digest.size() ? alphabet[value & 63] : '=';
   }
   return result;
}

bool validKey(std::string_view key)
{
   if (key.size() != 24 || key.substr(22) != "==" || std::string_view("AQgw").find(key[21]) == std::string_view::npos)
   {
      return false;
   }
   return std::ranges::all_of(key.substr(0, 22), [](unsigned char c) { return std::isalnum(c) || c == '+' || c == '/'; });
}

bool validUtf8(std::span<const uint8_t> text)
{
   for (size_t i = 0; i < text.size();)
   {
      const auto first = text[i++];
      if (first < 0x80)
      {
         continue;
      }
      const int count = first >= 0xc2 && first <= 0xdf ? 1 : first >= 0xe0 && first <= 0xef ? 2 : first >= 0xf0 && first <= 0xf4 ? 3 : -1;
      if (count < 0 || text.size() - i < static_cast<size_t>(count))
      {
         return false;
      }
      uint32_t value = first & ((1u << (6 - count)) - 1);
      for (int n = 0; n < count; ++n)
      {
         if ((text[i] & 0xc0) != 0x80)
         {
            return false;
         }
         value = (value << 6) | (text[i++] & 0x3f);
      }
      if ((count == 2 && value < 0x800) || (count == 3 && value < 0x10000) || (value >= 0xd800 && value <= 0xdfff) || value > 0x10ffff)
      {
         return false;
      }
   }
   return true;
}
}  // namespace

WebSocketByteStream::WebSocketByteStream(NetStreamSocketHandle socket, std::vector<std::string> origins)
    : _socket(std::move(socket)), _origins(std::move(origins))
{
}

bool WebSocketByteStream::sendRaw(std::span<const uint8_t> bytes)
{
   const int pending = NET_GetStreamSocketPendingWrites(_socket.get());
   if (pending < 0 || static_cast<size_t>(pending) > queue_limit || bytes.size() > queue_limit - static_cast<size_t>(pending) ||
       !NET_WriteToStreamSocket(_socket.get(), bytes.data(), static_cast<int>(bytes.size())))
   {
      _closed = true;
   }
   return !_closed;
}

void WebSocketByteStream::sendFrame(uint8_t opcode, std::span<const uint8_t> bytes)
{
   std::vector<uint8_t> frame{static_cast<uint8_t>(0x80 | opcode)};
   if (bytes.size() < 126)
   {
      frame.push_back(static_cast<uint8_t>(bytes.size()));
   }
   else
   {
      frame.push_back(bytes.size() <= 0xffff ? 126 : 127);
      for (int shift = bytes.size() <= 0xffff ? 8 : 56; shift >= 0; shift -= 8)
      {
         frame.push_back(static_cast<uint8_t>(uint64_t(bytes.size()) >> shift));
      }
   }
   frame.insert(frame.end(), bytes.begin(), bytes.end());
   sendRaw(frame);
}

void WebSocketByteStream::close(uint16_t code)
{
   if (!_closing && _upgraded)
   {
      const std::array<uint8_t, 2> status{static_cast<uint8_t>(code >> 8), static_cast<uint8_t>(code)};
      sendFrame(8, status);
   }
   _closing = true;
   _deadline = std::chrono::steady_clock::now() + std::chrono::seconds(1);
}

void WebSocketByteStream::handshake()
{
   const std::string_view request(reinterpret_cast<const char*>(_wire.data()), _wire.size());
   const auto end = request.find("\r\n\r\n");
   if (end == std::string_view::npos && request.size() <= 8192)
   {
      return;
   }
   bool valid = end != std::string_view::npos && end + 4 <= 8192;
   std::map<std::string, std::string> headers;
   auto line = request.find("\r\n");
   valid = valid && line != std::string_view::npos && request.substr(0, line).starts_with("GET /") &&
           request.substr(0, line).ends_with(" HTTP/1.1");
   while (valid && line < end)
   {
      const auto next = request.find("\r\n", line + 2);
      const auto field = request.substr(line + 2, next - line - 2);
      const auto colon = field.find(':');
      if (field.empty())
      {
         break;
      }
      if (colon == std::string_view::npos || colon == 0 ||
          !headers.emplace(lower(std::string(field.substr(0, colon))), trim(field.substr(colon + 1))).second)
      {
         valid = false;
      }
      line = next;
   }
   valid = valid && !headers["host"].empty() && lower(headers["upgrade"]) == "websocket" &&
           token(lower(headers["connection"]), "upgrade") && headers["sec-websocket-version"] == "13" &&
           validKey(headers["sec-websocket-key"]) && token(headers["sec-websocket-protocol"], "binary");
   const bool origin_allowed = std::ranges::find(_origins, headers["origin"]) != _origins.end();
   if (!valid || !origin_allowed)
   {
      const std::string response = origin_allowed ? "HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\nConnection: close\r\n\r\n"
                                                  : "HTTP/1.1 403 Forbidden\r\nContent-Length: 0\r\nConnection: close\r\n\r\n";
      sendRaw({reinterpret_cast<const uint8_t*>(response.data()), response.size()});
      close(1002);
      return;
   }
   const std::string response =
      "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Protocol: "
      "binary\r\nSec-WebSocket-Accept: " +
      acceptKey(headers["sec-websocket-key"]) + "\r\n\r\n";
   sendRaw({reinterpret_cast<const uint8_t*>(response.data()), response.size()});
   _upgraded = true;
   _wire.erase(_wire.begin(), _wire.begin() + static_cast<std::ptrdiff_t>(end + 4));
}

bool WebSocketByteStream::receiveFrame()
{
   if (_wire.size() < 2)
   {
      return false;
   }
   const uint8_t opcode = _wire[0] & 15;
   const bool final = (_wire[0] & 0x80) != 0;
   uint64_t length = _wire[1] & 127;
   const bool control = opcode >= 8;
   if ((_wire[0] & 0x70) || !(_wire[1] & 0x80) || (opcode != 0 && opcode != 2 && opcode != 8 && opcode != 9 && opcode != 10) ||
       (control && (!final || length > 125)) || (!control && ((opcode == 0) != _fragmented)))
   {
      close(opcode == 1 ? 1003 : 1002);
      return false;
   }
   size_t offset = 2;
   const auto marker = length;
   if (marker >= 126)
   {
      const size_t count = marker == 126 ? 2 : 8;
      if (_wire.size() < offset + count)
      {
         return false;
      }
      length = 0;
      for (size_t i = 0; i < count; ++i)
      {
         length = (length << 8) | _wire[offset++];
      }
      if ((marker == 126 && length < 126) || (marker == 127 && (length <= 0xffff || (length >> 63))))
      {
         close(1002);
         return false;
      }
   }
   if (length > message_limit || (!control && length > message_limit - _message.size()))
   {
      close(1009);
      return false;
   }
   if (_wire.size() < offset + 4 + length)
   {
      return false;
   }
   const std::array<uint8_t, 4> mask{_wire[offset], _wire[offset + 1], _wire[offset + 2], _wire[offset + 3]};
   offset += 4;
   std::vector<uint8_t> payload(static_cast<size_t>(length));
   for (size_t i = 0; i < payload.size(); ++i)
   {
      payload[i] = _wire[offset + i] ^ mask[i % 4];
   }
   _wire.erase(_wire.begin(), _wire.begin() + static_cast<std::ptrdiff_t>(offset + payload.size()));
   if (opcode == 8)
   {
      if (payload.size() == 1)
      {
         close(1002);
      }
      else if (payload.size() >= 2)
      {
         const auto code = (uint16_t(payload[0]) << 8) | payload[1];
         if (!((code >= 1000 && code <= 1014 && code != 1004 && code != 1005 && code != 1006) || (code >= 3000 && code <= 4999)))
         {
            close(1002);
         }
         else if (!validUtf8(std::span(payload).subspan(2)))
         {
            close(1007);
         }
         else
         {
            close(code);
         }
      }
      else
      {
         close(1000);
      }
   }
   else if (opcode == 9)
   {
      sendFrame(10, payload);
   }
   else if (!control)
   {
      _message.insert(_message.end(), payload.begin(), payload.end());
      _fragmented = !final;
      if (final)
      {
         if (_message.size() > queue_limit - _received.size())
         {
            close(1009);
         }
         else
         {
            _received.insert(_received.end(), _message.begin(), _message.end());
         }
         _message.clear();
      }
   }
   return !_closing;
}

int32_t WebSocketByteStream::read(std::span<char> buffer)
{
   if (_closed)
   {
      return -1;
   }
   if (_closing)
   {
      return NET_GetStreamSocketPendingWrites(_socket.get()) <= 0 || std::chrono::steady_clock::now() >= _deadline ? -1 : 0;
   }
   if (!_upgraded && std::chrono::steady_clock::now() >= _deadline)
   {
      return -1;
   }
   std::array<uint8_t, 16384> chunk{};
   const int count = NET_ReadFromStreamSocket(_socket.get(), chunk.data(), static_cast<int>(chunk.size()));
   if (count < 0)
   {
      return -1;
   }
   _wire.insert(_wire.end(), chunk.begin(), chunk.begin() + count);
   if (_wire.size() > message_limit + chunk.size())
   {
      close(1009);
      return 0;
   }
   if (!_upgraded)
   {
      handshake();
   }
   if (_upgraded)
   {
      for (int frames = 0; frames < 64 && !_closing && receiveFrame(); ++frames)
      {
      }
   }
   const auto copied = std::min(buffer.size(), _received.size());
   std::copy_n(_received.begin(), copied, buffer.begin());
   _received.erase(_received.begin(), _received.begin() + static_cast<std::ptrdiff_t>(copied));
   return static_cast<int32_t>(copied);
}

void WebSocketByteStream::write(std::span<const uint8_t> bytes)
{
   if (_upgraded && !_closed && !_closing)
   {
      sendFrame(2, bytes);
   }
}

std::string WebSocketByteStream::getPeerAddress() const
{
   const NetAddressHandle address(NET_GetStreamSocketAddress(_socket.get()), &NET_UnrefAddress);
   return address ? NET_GetAddressString(address.get()) : "?";
}

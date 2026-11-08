#include "clientconnection.h"

#include <chrono>

namespace
{
class LocalConnection final : public ClientConnection
{
public:
   explicit LocalConnection(std::unique_ptr<ByteStream> stream) : _stream(std::move(stream)) {}
   ConnectionState poll() override { return _state; }
   int32_t read(std::span<char> buffer) override
   {
      const auto count = _stream->read(buffer);
      if (count < 0)
         _state = ConnectionState::Failed;
      return count;
   }
   void write(std::span<const uint8_t> bytes) override { _stream->write(bytes); }
   std::string getPeerAddress() const override { return _stream->getPeerAddress(); }
   std::string_view error() const override { return "Local game server disconnected"; }
   void drain(int32_t) override {}

private:
   std::unique_ptr<ByteStream> _stream;
   ConnectionState _state = ConnectionState::Connected;
};
}

std::unique_ptr<ClientConnection> connectToLocalStream(std::unique_ptr<ByteStream> stream)
{
   return std::make_unique<LocalConnection>(std::move(stream));
}

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#include <emscripten/websocket.h>

#include <algorithm>
#include <cstdlib>
#include <deque>

// A deployment may supply ?server=wss://example.org/dynablaster. All players and
// bots use the same endpoint, including story mode's native loopback host name.
// clang-format off
EM_JS(bool, dynablaster_remote_transport, (const char* host), {
   const name = UTF8ToString(host).toLowerCase();
   return !!new URLSearchParams(location.search).get('server') ||
      !["", 'localhost', '127.0.0.1', '::1'].includes(name);
});
EM_JS(char*, dynablaster_websocket_url, (const char* host), {
   const configured = new URLSearchParams(location.search).get('server');
   const name = UTF8ToString(host);
   const url = configured || (name.startsWith('ws://') || name.startsWith('wss://')
                                  ? name
                                  : (location.protocol === 'https:' ? 'wss://' : 'ws://') +
                                        (name.includes(':') && !name.startsWith('[') ? '[' + name + ']' : name) + ':8081/');
   return stringToNewUTF8(url);
});

// Invalid URLs and mixed-content/CSP restrictions can throw a JavaScript exception.
// Catch it at the import boundary so a connection error cannot abort the game.
EM_JS(int, dynablaster_websocket_create, (intptr_t create, intptr_t attributes), {
   try
   {
      return getWasmTableEntry(create)(attributes);
   }
   catch (error)
   {
      return -1;
   }
});
// clang-format on

namespace
{
class WebSocketConnection final : public ClientConnection
{
  public:
   explicit WebSocketConnection(const std::string& host)
   {
      char* url = dynablaster_websocket_url(host.c_str());
      _url = url;
      std::free(url);
      if (!emscripten_websocket_is_supported() || (!_url.starts_with("ws://") && !_url.starts_with("wss://")))
      {
         fail("A ws:// or wss:// game bridge URL is required");
         return;
      }
      EmscriptenWebSocketCreateAttributes attributes{};
      attributes.url = _url.c_str();
      attributes.protocols = "binary";
      attributes.createOnMainThread = true;
      _socket =
          dynablaster_websocket_create(reinterpret_cast<intptr_t>(&emscripten_websocket_new), reinterpret_cast<intptr_t>(&attributes));
      if (_socket <= 0)
      {
         fail("Cannot open the game WebSocket; check the bridge URL and HTTPS/WSS settings");
         return;
      }
      emscripten_websocket_set_onopen_callback(_socket, this,
                                               [](int, const EmscriptenWebSocketOpenEvent*, void* data)
                                               {
                                                  auto& self = *static_cast<WebSocketConnection*>(data);
                                                  if (self._state == ConnectionState::Connecting)
                                                     self._state = ConnectionState::Connected;
                                                  return true;
                                               });
      emscripten_websocket_set_onmessage_callback(_socket, this,
                                                  [](int, const EmscriptenWebSocketMessageEvent* event, void* data)
                                                  {
                                                     auto& self = *static_cast<WebSocketConnection*>(data);
                                                     if (event->isText || event->numBytes > buffer_limit - self._incoming.size())
                                                        self.fail("Invalid or oversized game bridge message");
                                                     else if (self._state == ConnectionState::Connected)
                                                        self._incoming.insert(self._incoming.end(), event->data,
                                                                              event->data + event->numBytes);
                                                     return true;
                                                  });
      emscripten_websocket_set_onerror_callback(
          _socket, this,
          [](int, const EmscriptenWebSocketErrorEvent*, void* data)
          {
             static_cast<WebSocketConnection*>(data)->fail("Game bridge connection failed; check its URL, origin and TLS configuration");
             return true;
          });
      emscripten_websocket_set_onclose_callback(_socket, this,
                                                [](int, const EmscriptenWebSocketCloseEvent*, void* data)
                                                {
                                                   static_cast<WebSocketConnection*>(data)->fail("Game bridge connection closed");
                                                   return true;
                                                });
   }

   ~WebSocketConnection() override
   {
      if (_socket > 0)
      {
         // Emscripten's delete removes every JS event handler before this object
         // is freed, including close/error events queued by an unfinished handshake.
         emscripten_websocket_close(_socket, 1000, "");
         emscripten_websocket_delete(_socket);
      }
   }

   ConnectionState poll() override
   {
      if (_state == ConnectionState::Connecting && std::chrono::steady_clock::now() - _started > std::chrono::seconds(10))
         fail("Game bridge connection timed out");
      return _state;
   }

   int32_t read(std::span<char> buffer) override
   {
      const auto count = std::min(buffer.size(), _incoming.size());
      std::copy_n(_incoming.begin(), count, buffer.begin());
      _incoming.erase(_incoming.begin(), _incoming.begin() + static_cast<std::ptrdiff_t>(count));
      return count ? static_cast<int32_t>(count) : (_state == ConnectionState::Failed ? -1 : 0);
   }

   void write(std::span<const uint8_t> bytes) override
   {
      if (_state != ConnectionState::Connected)
         return;
      size_t queued = 0;
      if (emscripten_websocket_get_buffered_amount(_socket, &queued) != EMSCRIPTEN_RESULT_SUCCESS || queued > buffer_limit ||
          bytes.size() > buffer_limit - queued ||
          emscripten_websocket_send_binary(_socket, const_cast<uint8_t*>(bytes.data()), static_cast<uint32_t>(bytes.size())) !=
              EMSCRIPTEN_RESULT_SUCCESS)
         fail("Game bridge send failed or its outgoing buffer is full");
   }

   std::string getPeerAddress() const override { return _url; }
   std::string_view error() const override { return _error; }
   // The browser event loop must keep running to flush WebSocket frames.
   void drain(int32_t) override {}

  private:
   void fail(std::string reason)
   {
      if (_state != ConnectionState::Failed)
         _error = std::move(reason);
      _state = ConnectionState::Failed;
   }

   static constexpr size_t buffer_limit = 8 * 1024 * 1024;
   EMSCRIPTEN_WEBSOCKET_T _socket = 0;
   ConnectionState _state = ConnectionState::Connecting;
   std::chrono::steady_clock::time_point _started = std::chrono::steady_clock::now();
   std::string _url;
   std::string _error;
   std::deque<char> _incoming;
};
}  // namespace

std::unique_ptr<ClientConnection> connectToHost(const std::string& host, uint16_t /*port*/)
{
   return std::make_unique<WebSocketConnection>(host);
}

bool usesRemoteTransport(const std::string& host)
{
   return dynablaster_remote_transport(host.c_str());
}

#else

#include "nethandles.h"

namespace
{
class TcpConnection final : public ClientConnection
{
  public:
   TcpConnection(const std::string& host, uint16_t port) : _host(host), _port(port)
   {
      _address.reset(NET_ResolveHostname(host.c_str()));
      if (!_address)
         fail();
   }

   ConnectionState poll() override
   {
      if (_state == ConnectionState::Failed)
         return _state;
      if (_state == ConnectionState::Connecting && std::chrono::steady_clock::now() - _started > std::chrono::seconds(10))
      {
         _error = "Game server connection timed out";
         _state = ConnectionState::Failed;
         return _state;
      }
      if (_address)
      {
         const auto status = NET_GetAddressStatus(_address.get());
         if (status == NET_FAILURE)
            fail();
         else if (status == NET_SUCCESS)
         {
            _socket.reset(NET_CreateClient(_address.get(), _port, 0));
            _address.reset();
            if (!_socket)
               fail();
         }
      }
      if (_socket && _state == ConnectionState::Connecting)
      {
         const auto status = NET_GetConnectionStatus(_socket.get());
         if (status == NET_FAILURE)
            fail();
         else if (status == NET_SUCCESS)
            _state = ConnectionState::Connected;
      }
      return _state;
   }

   int32_t read(std::span<char> buffer) override
   {
      const int32_t count = NET_ReadFromStreamSocket(_socket.get(), buffer.data(), static_cast<int>(buffer.size()));
      if (count < 0)
         fail();
      return count;
   }

   void write(std::span<const uint8_t> bytes) override
   {
      if (_state == ConnectionState::Connected && !NET_WriteToStreamSocket(_socket.get(), bytes.data(), static_cast<int>(bytes.size())))
         fail();
   }

   std::string getPeerAddress() const override { return _host; }
   std::string_view error() const override { return _error; }
   void drain(int32_t timeout_ms) override
   {
      if (_socket && _state == ConnectionState::Connected)
         NET_WaitUntilStreamSocketDrained(_socket.get(), timeout_ms);
   }

  private:
   void fail()
   {
      _error = SDL_GetError();
      _state = ConnectionState::Failed;
   }

   std::string _host;
   uint16_t _port;
   NetAddressHandle _address{nullptr, &NET_UnrefAddress};
   NetStreamSocketHandle _socket{nullptr, &NET_DestroyStreamSocket};
   ConnectionState _state = ConnectionState::Connecting;
   std::chrono::steady_clock::time_point _started = std::chrono::steady_clock::now();
   std::string _error;
};
}  // namespace

std::unique_ptr<ClientConnection> connectToHost(const std::string& host, uint16_t port)
{
   return std::make_unique<TcpConnection>(host, port);
}

bool usesRemoteTransport(const std::string&)
{
   return true;
}
#endif

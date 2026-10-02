// the join request's optional preferred color: the packet without one is unchanged on the wire,
// the server grants a free preferred color and falls back to the next free one if it's taken.
#include "binaryreader.h"
#include "game/bombermanclient.h"
#include "joingamerequestpacket.h"
#include "joingameresponsepacket.h"
#include "loginrequestpacket.h"
#include "loginresponsepacket.h"
#include "packetstreambuffer.h"
#include "playerinfo.h"
#include "timer.h"

#include <SDL3/SDL.h>
#include <SDL3_net/SDL_net.h>

#include <array>
#include <functional>
#include <memory>
#include <optional>
#include <vector>

namespace
{
bool waitFor(const std::function<bool()>& condition, uint64_t timeout_ms = 10000)
{
   const uint64_t start = SDL_GetTicks();
   while (!condition())
   {
      if (SDL_GetTicks() - start > timeout_ms)
      {
         return false;
      }
      Timer::update();
      SDL_Delay(1);
   }
   return true;
}

bool check(bool condition, const char* what)
{
   if (!condition)
   {
      SDL_Log("FAILED: %s", what);
   }
   return condition;
}

/// \brief a bare connection that logs in, joins and reports the color it got
class RawPlayer
{
public:
   RawPlayer(int32_t game_id, std::optional<Constants::Color> preferred_color) : _game_id(game_id), _preferred_color(preferred_color)
   {
      NET_Address* address = NET_ResolveHostname("127.0.0.1");
      NET_WaitUntilResolved(address, -1);
      _socket = NET_CreateClient(address, 6300, 0);
      NET_UnrefAddress(address);
      NET_WaitUntilConnected(_socket, -1);

      LoginRequestPacket login("raw", false);
      send(login);
   }

   ~RawPlayer()
   {
      NET_DestroyStreamSocket(_socket);
   }

   void poll()
   {
      std::array<char, 4096> chunk{};
      int32_t bytes_read = 0;
      while ((bytes_read = NET_ReadFromStreamSocket(_socket, chunk.data(), static_cast<int>(chunk.size()))) > 0)
      {
         _buffer.append(chunk.data(), static_cast<size_t>(bytes_read));
      }

      while (true)
      {
         if (_block_size == 0)
         {
            if (_buffer.bytesAvailable() < sizeof(uint16_t))
            {
               break;
            }
            BinaryReader size_reader = _buffer.reader();
            size_reader >> _block_size;
            _buffer.consume(size_reader.pos());
         }
         if (_buffer.bytesAvailable() < _block_size)
         {
            break;
         }

         BinaryReader in = _buffer.reader(_block_size);
         auto packet = Packet::deserialize(in);
         _buffer.consume(_block_size);
         _block_size = 0;

         if (packet && packet->getType() == Packet::LOGINRESPONSE)
         {
            _player_id = static_cast<LoginResponsePacket*>(packet.get())->getId();
            JoinGameRequestPacket join(_game_id, _preferred_color);
            send(join);
         }
         else if (packet && packet->getType() == Packet::JOINGAMERESPONSE)
         {
            auto* response = static_cast<JoinGameResponsePacket*>(packet.get());
            if (response->getPlayerId() == _player_id)
            {
               _color = response->getColor();
            }
         }
      }
      _buffer.compact();
   }

   std::optional<Constants::Color> getColor() const
   {
      return _color;
   }

private:
   void send(Packet& packet)
   {
      packet.serialize();
      NET_WriteToStreamSocket(_socket, packet.constData(), static_cast<int>(packet.size()));
   }

   int32_t _game_id = 0;
   std::optional<Constants::Color> _preferred_color;
   NET_StreamSocket* _socket = nullptr;
   PacketStreamBuffer _buffer;
   uint16_t _block_size = 0;
   int32_t _player_id = -1;
   std::optional<Constants::Color> _color;
};

// players stay connected so the colors they hold stay taken
std::vector<std::unique_ptr<RawPlayer>> players;

std::optional<Constants::Color> joinAs(int32_t game_id, std::optional<Constants::Color> preferred_color)
{
   auto& player = players.emplace_back(std::make_unique<RawPlayer>(game_id, preferred_color));
   waitFor(
      [&]()
      {
         player->poll();
         return player->getColor().has_value();
      }
   );
   return player->getColor();
}
}  // namespace

int main(int /*argc*/, char** /*argv*/)
{
   bool ok = true;

   // on the wire: no preference is the old layout, a preference adds one int32
   {
      JoinGameRequestPacket plain(3);
      plain.serialize();
      JoinGameRequestPacket colored(3, Constants::ColorCyan);
      colored.serialize();
      ok &= check(plain.size() == sizeof(uint16_t) + sizeof(uint8_t) + sizeof(int32_t) + sizeof(int32_t), "plain request unchanged");
      ok &= check(colored.size() == plain.size() + sizeof(int32_t), "preferred color appended");

      BinaryReader in(reinterpret_cast<const uint8_t*>(colored.constData()) + sizeof(uint16_t), colored.size() - sizeof(uint16_t));
      auto packet = Packet::deserialize(in);
      auto* request = dynamic_cast<JoinGameRequestPacket*>(packet.get());
      ok &= check(request && request->getId() == 3 && request->getPreferredColor() == Constants::ColorCyan, "round trip");
   }

   if (!NET_Init())
   {
      SDL_Log("FAILED: SDL_net unavailable: %s", SDL_GetError());
      return 1;
   }

   {
      BombermanClient client;
      client.initialize();

      bool joined = false;
      int32_t login_responses = 0;
      client.setPreferredColor(Constants::ColorRed);
      client.loginResponseSignal.connect(
         [&](bool success)
         {
            login_responses++;
            if (success)
            {
               client.createGameAutomatic();
            }
         }
      );
      client.createGameResponseSignal.connect(
         [&](bool success, int game_id, bool)
         {
            if (success)
            {
               // a rename before joining, as the controls page does for the main player
               client.rename("renamed");
               client.joinGame(game_id);
            }
         }
      );
      client.joinGameResponseSignal.connect([&](bool success) { joined = success; });

      client.host();
      client.loginRequest("127.0.0.1", "main");
      ok &= check(waitFor([&]() { return joined; }), "main client joins");
      ok &= check(
         client.getCurrentPlayerInfo() && client.getCurrentPlayerInfo()->getColor() == Constants::ColorRed, "free preferred color granted"
      );
      ok &= check(
         client.getCurrentPlayerInfo() && client.getCurrentPlayerInfo()->getNick() == "renamed",
         "the rename reached the server before the join"
      );
      ok &= check(login_responses == 1, "the rename doesn't count as a new login");

      const int32_t game_id = client.getGameId();
      ok &= check(joinAs(game_id, Constants::ColorRed) == Constants::ColorWhite, "taken preferred color falls back to the next free one");
      ok &= check(joinAs(game_id, Constants::ColorCyan) == Constants::ColorCyan, "another free preferred color granted");
      ok &= check(joinAs(game_id, std::nullopt) == Constants::ColorBlack, "no preference takes the next free one");

      players.clear();
      client.leaveGameRequest();
   }

   NET_Quit();
   SDL_Log(ok ? "preferred color test passed" : "preferred color test failed");
   return ok ? 0 : 1;
}

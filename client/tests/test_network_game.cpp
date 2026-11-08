#include "clientconnection.h"
#include "creategamerequestpacket.h"
#include "creategameresponsepacket.h"
#include "joingamerequestpacket.h"
#include "joingameresponsepacket.h"
#include "keypacket.h"
#include "loginrequestpacket.h"
#include "loginresponsepacket.h"
#include "mapitemcreatedpacket.h"
#include "packetstreambuffer.h"
#include "playersynchronizepacket.h"
#include "positionpacket.h"
#include "startgamerequestpacket.h"
#include "startgameresponsepacket.h"
#include "server.h"
#include "timer.h"

#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <optional>
#include <thread>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
#include <SDL3_net/SDL_net.h>
#endif

namespace
{
bool expect_failure = false;
std::unique_ptr<Server> local_server;
std::function<void()> cleanup;

std::unique_ptr<ClientConnection> createConnection()
{
   if (!local_server)
      return connectToHost("127.0.0.1", SERVER_PORT);
   auto [client, server] = LoopbackByteStream::createPair();
   local_server->connectLocal(std::move(server));
   return connectToLocalStream(std::move(client));
}
[[noreturn]] void finish(int status, const char* message)
{
   std::puts(message);
   if (cleanup)
      cleanup();
#ifdef __EMSCRIPTEN__
   emscripten_force_exit(status);
#else
   std::exit(status);
#endif
}

struct Peer
{
   std::unique_ptr<ClientConnection> connection = createConnection();
   PacketStreamBuffer buffer;
   int32_t id = -1;
   bool login_sent = false;
   bool joined = false;

   void send(Packet& packet)
   {
      packet.serialize();
      // Exercise reassembly: the length prefix itself crosses transport messages.
      connection->write(std::span(packet).first(1));
      connection->write(std::span(packet).subspan(1));
   }
};

struct Probe
{
   std::array<Peer, 2> peers;
   int32_t game = -1;
   bool create_sent = false;
   bool join_sent = false;
   bool start_sent = false;
   bool started = false;
   bool bomb_seen = false;
   bool moved = false;
   bool finished = false;
   std::optional<float> start_x;
   std::chrono::steady_clock::time_point began = std::chrono::steady_clock::now();

   void packet(Peer& peer, const Packet& packet)
   {
      switch (packet.getType())
      {
         case Packet::LOGINRESPONSE:
         {
            const auto& login = static_cast<const LoginResponsePacket&>(packet);
            if (!login.isBroadcast())
               peer.id = login.getId();
            break;
         }
         case Packet::CREATEGAMERESPONSE:
            game = static_cast<const CreateGameResponsePacket&>(packet).getGameInformation().getId();
            break;
         case Packet::JOINGAMERESPONSE:
         {
            const auto& join = static_cast<const JoinGameResponsePacket&>(packet);
            if (join.getPlayerId() == peer.id && join.isSuccessful())
               peer.joined = true;
            break;
         }
         case Packet::STARTGAMERESPONSE:
            if (static_cast<const StartGameResponsePacket&>(packet).isStarted() && !started)
            {
               started = true;
               KeyPacket keys(0, Constants::KeyRight | Constants::KeyBomb);
               peers[0].send(keys);
            }
            break;
         case Packet::POSITION:
         {
            const auto& position = static_cast<const PositionPacket&>(packet);
            if (&peer == &peers[0] && position.getPlayerId() == peer.id)
            {
               if (!start_x)
                  start_x = position.getX();
               else if (started && position.getX() > *start_x + 0.1f)
                  moved = true;
            }
            break;
         }
         case Packet::MAPITEMCREATED:
            if (started && static_cast<const MapItemCreatedPacket&>(packet).getItemType() == MapItem::Bomb)
               bomb_seen = true;
            break;
         case Packet::ERROR:
            finish(1, "FAIL: game server returned an error packet");
         default:
            break;
      }
   }

   void tick()
   {
      if (std::chrono::steady_clock::now() - began > std::chrono::seconds(40))
      {
         std::printf("state: game=%d joined=%d/%d started=%d moved=%d bomb=%d\n", game, peers[0].joined, peers[1].joined, started, moved,
                     bomb_seen);
         finish(1, "FAIL: network game timed out");
      }
      for (size_t i = 0; i < peers.size(); ++i)
      {
         auto& peer = peers[i];
         const auto state = peer.connection->poll();
         if (state == ConnectionState::Failed)
         {
            std::printf("%s\n", std::string(peer.connection->error()).c_str());
            if (expect_failure)
               finish(0, "PASS: connection failure was reported without aborting the runtime");
            finish(1, "FAIL: network connection failed");
         }
         if (state != ConnectionState::Connected)
            continue;
         if (!peer.login_sent)
         {
            LoginRequestPacket login(i == 0 ? "network-host" : "network-guest", false);
            peer.send(login);
            peer.login_sent = true;
         }
         // Tiny reads force packets to span many reads, regardless of WS framing.
         std::array<char, 7> bytes{};
         int32_t count;
         while ((count = peer.connection->read(bytes)) > 0)
            peer.buffer.append(std::span(bytes).first(static_cast<size_t>(count)));
         if (count < 0)
            finish(1, "FAIL: game connection closed prematurely");
         while (peer.buffer.bytesAvailable() >= 2)
         {
            auto header = peer.buffer.reader();
            uint16_t size = 0;
            header >> size;
            if (peer.buffer.bytesAvailable() < static_cast<size_t>(size) + 2)
               break;
            peer.buffer.consume(2);
            auto reader = peer.buffer.reader(size);
            auto received = Packet::deserialize(reader);
            if (!received || !reader.ok())
               finish(1, "FAIL: invalid game packet");
            peer.buffer.consume(size);
            packet(peer, *received);
         }
         peer.buffer.compact();
      }
      if (!create_sent && peers[0].id >= 0 && peers[1].id >= 0)
      {
         CreateGameRequestPacket create("network-integration", "level-castle", 1, 60, 2, true, true, true, true, false,
                                        Constants::Dimension13x11);
         peers[0].send(create);
         create_sent = true;
      }
      if (game >= 0 && !join_sent)
      {
         JoinGameRequestPacket join(game);
         for (auto& peer : peers)
            peer.send(join);
         join_sent = true;
      }
      if (peers[0].joined && peers[1].joined && !start_sent)
      {
         PlayerSynchronizePacket sync(PlayerSynchronizePacket::LevelLoaded);
         for (auto& peer : peers)
            peer.send(sync);
         StartGameRequestPacket start(game);
         peers[0].send(start);
         start_sent = true;
      }
      if (started && moved && bomb_seen)
         finished = true;
   }
};

std::unique_ptr<Probe> probe;
int matches = 0;

void tickProbe()
{
   Timer::update();
   probe->tick();
   if (probe->finished)
   {
      // Reconnect in the same runtime to exercise callback and socket cleanup.
      probe.reset();
      if (++matches == 2)
         finish(0, "PASS: two clients completed login, create/join/start, movement and bombing twice, with disconnect/reconnect");
      probe = std::make_unique<Probe>();
   }
}
}  // namespace

int main(int argc, char** argv)
{
   expect_failure = argc > 1 && std::string_view(argv[1]) == "--expect-failure";
#ifndef __EMSCRIPTEN__
   if (!NET_Init())
      finish(1, "FAIL: NET_Init");
#endif
   if (argc > 1 && std::string_view(argv[1]) == "--embedded")
   {
      local_server = std::make_unique<Server>(false);
      local_server->startPolling();
   }
   probe = std::make_unique<Probe>();
   cleanup = [] { probe.reset(); local_server.reset(); };
#ifdef __EMSCRIPTEN__
   emscripten_set_main_loop(tickProbe, 0, true);
#else
   while (true)
   {
      tickProbe();
      std::this_thread::sleep_for(std::chrono::milliseconds(5));
   }
#endif
}

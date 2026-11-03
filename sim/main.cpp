// dynablaster_sim - headless bot matches for measuring and training bots, NOT shipped with the
// game. Runs the real server and real bot clients in one process, wired through in-memory
// streams, on simulated time that advances as fast as the cpu allows. Writes one csv row per
// player and round.
//
//    dynablaster_sim [--rounds n] [--bots n] [--duration s] [--step ms] [--out file.csv] [--seed n] [--realtime] [--verbose]
//
// --seed makes a run repeatable, --realtime plays on the real clock like the game does, to compare against simulated time.
//
// run it from the client's build directory, the server reads its settings from data/.

// sim
#include "simrecorder.h"

// server
#include "game.h"
#include "server.h"

// ai
#include "botfactory.h"

// shared
#include "bytestream.h"
#include "clock.h"
#include "creategamerequestpacket.h"
#include "creategameresponsepacket.h"
#include "log.h"
#include "packet.h"
#include "packetstreambuffer.h"
#include "random.h"
#include "startgamerequestpacket.h"
#include "timer.h"

// stdlib
#include <array>
#include <charconv>
#include <chrono>
#include <cstdlib>
#include <exception>
#include <format>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <thread>

namespace
{

struct Options
{
   int32_t _rounds = 10;
   int32_t _bots = 4;
   int32_t _duration_s = 180;
   int32_t _step_ms = 2;
   std::string _out = "sim.csv";
   bool _verbose = false;
   bool _realtime = false;
   std::optional<uint32_t> _seed;
};

std::optional<int32_t> parseInt(std::string_view text)
{
   int32_t value = 0;
   const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);

   if (error != std::errc{} || end != text.data() + text.size())
   {
      return std::nullopt;
   }

   return value;
}

std::optional<Options> parseOptions(int argc, char** argv)
{
   Options options;

   for (int i = 1; i < argc; i++)
   {
      const std::string_view name = argv[i];

      if (name == "--verbose")
      {
         options._verbose = true;
         continue;
      }

      if (name == "--realtime")
      {
         options._realtime = true;
         continue;
      }

      if (i + 1 >= argc)
      {
         return std::nullopt;
      }

      const std::string_view value = argv[++i];

      if (name == "--out")
      {
         options._out = value;
         continue;
      }

      const auto number = parseInt(value);

      if (!number || *number < 0)
      {
         return std::nullopt;
      }

      if (name == "--seed")
      {
         options._seed = static_cast<uint32_t>(*number);
         continue;
      }

      if (*number == 0)
      {
         return std::nullopt;
      }

      if (name == "--rounds")
      {
         options._rounds = *number;
      }
      else if (name == "--bots")
      {
         options._bots = *number;
      }
      else if (name == "--duration")
      {
         options._duration_s = *number;
      }
      else if (name == "--step")
      {
         options._step_ms = *number;
      }
      else
      {
         return std::nullopt;
      }
   }

   if (options._bots < 2)
   {
      return std::nullopt;
   }

   return options;
}

// the client that creates and starts the game without joining it, the bots play
class Controller
{
public:
   explicit Controller(std::unique_ptr<ByteStream> stream) : _stream(std::move(stream))
   {
   }

   void send(Packet& packet)
   {
      packet.serialize();
      _stream->write(packet);
   }

   //! reads what arrived, returns the created game's id once the server answered
   std::optional<int32_t> pollCreatedGame()
   {
      std::array<char, 4096> chunk{};
      int32_t bytes_read = 0;

      while ((bytes_read = _stream->read(chunk)) > 0)
      {
         _buffer.append(std::span(chunk).first(static_cast<size_t>(bytes_read)));
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

         BinaryReader in = _buffer.reader();
         const auto packet = Packet::deserialize(in);
         _buffer.consume(in.pos());
         _block_size = 0;

         if (packet && packet->getType() == Packet::CREATEGAMERESPONSE)
         {
            _game_id = static_cast<const CreateGameResponsePacket&>(*packet).getGameInformation().getId();
         }
      }

      _buffer.compact();
      return _game_id;
   }

private:
   std::unique_ptr<ByteStream> _stream;
   PacketStreamBuffer _buffer;
   uint16_t _block_size = 0;
   std::optional<int32_t> _game_id;
};

int run(int argc, char** argv)
{
   const auto parsed = parseOptions(argc, argv);

   if (!parsed)
   {
      std::cerr << "usage: dynablaster_sim [--rounds n] [--bots n>=2] [--duration s] [--step ms] [--out file.csv] [--seed n] [--realtime] [--verbose]\n";
      return 2;
   }

   const Options& options = *parsed;

   if (!options._verbose)
   {
      Log::setMinimumLevel(Log::Level::Warning);
   }

   if (!options._realtime)
   {
      Clock::enableSimulation();
   }

   if (options._seed)
   {
      Random::seed(*options._seed);
   }

   const auto simulation_start = Clock::now();
   const auto wall_start = std::chrono::steady_clock::now();

   Server server(false);
   server.startPolling();

   const auto connect = [&server]()
   {
      auto [client_end, server_end] = LoopbackByteStream::createPair();
      server.connectLocal(std::move(server_end));
      return std::move(client_end);
   };

   int64_t steps = 0;

   const auto step = [&options, &steps]()
   {
      if (options._realtime)
      {
         // sleep_for(1 ms) lasts a whole scheduler tick (~16 ms) on windows unless the timer
         // resolution was raised, which the game does through sdl; yield until the next ms instead
         const auto due = std::chrono::steady_clock::now() + std::chrono::milliseconds(1);

         while (std::chrono::steady_clock::now() < due)
         {
            std::this_thread::yield();
         }
      }
      else
      {
         Clock::advance(std::chrono::milliseconds(options._step_ms));
      }

      Timer::update();
      steps++;
   };

   // a round can't outlast its duration plus countdown and winner display by much
   const auto time_limit = std::chrono::seconds((options._duration_s + 30) * options._rounds + 60);
   const auto timed_out = [&]() { return Clock::now() - simulation_start > time_limit; };

   Controller controller(connect());

   CreateGameRequestPacket create("sim", "castle", options._rounds, options._duration_s, options._bots, true, true, true, true, false, Constants::Dimension13x11);
   controller.send(create);

   std::optional<int32_t> game_id;

   while (!(game_id = controller.pollCreatedGame()) && !timed_out())
   {
      step();
   }

   if (!game_id)
   {
      std::cerr << "the server never created the game\n";
      return 1;
   }

   auto game = server.getGame(*game_id);

   if (!game)
   {
      std::cerr << "the created game is gone\n";
      return 1;
   }

   SimRecorder recorder(game->get(), options._rounds);

   BotFactory bots;
   bots.setGameId(*game_id);
   bots.setLocalConnector(connect);
   bots.add(options._bots);

   while (static_cast<int32_t>(game->get().getPlayers().size()) < options._bots && !timed_out())
   {
      step();
   }

   StartGameRequestPacket start(*game_id);
   controller.send(start);

   while (!recorder.isFinished() && !timed_out())
   {
      step();
   }

   const auto simulated_s = std::chrono::duration<double>(Clock::now() - simulation_start).count();
   const auto wall_s = std::chrono::duration<double>(std::chrono::steady_clock::now() - wall_start).count();

   if (!recorder.write(options._out))
   {
      std::cerr << std::format("can't write {}\n", options._out);
      return 1;
   }

   std::cout << recorder.summary();
   std::cout << std::format(
      "{} of {} rounds, {:.0f} s simulated in {:.1f} s ({:.0f}x real time), {:.2f} ms per tick, rows in {}\n",
      recorder.getRoundsPlayed(),
      options._rounds,
      simulated_s,
      wall_s,
      wall_s > 0.0 ? simulated_s / wall_s : 0.0,
      steps > 0 ? simulated_s * 1000.0 / static_cast<double>(steps) : 0.0,
      options._out
   );

   return recorder.isFinished() ? 0 : 1;
}

}  // namespace

int main(int argc, char** argv)
{
   try
   {
      return run(argc, argv);
   }
   catch (const std::exception& exception)
   {
      std::cerr << std::format("simulation aborted: {}\n", exception.what());
      return 1;
   }
}

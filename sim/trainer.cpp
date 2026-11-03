#include "trainer.h"

// ai
#include "learnedbotweights.h"

#include <algorithm>
#include <atomic>
#include <charconv>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <fstream>
#include <functional>
#include <iostream>
#include <numeric>
#include <optional>
#include <random>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace
{

struct Options
{
   int32_t _generations = 50;
   int32_t _population = 24;
   int32_t _rounds = 20;
   int32_t _duration_s = 60;
   int32_t _bots = 4;
   int32_t _learned = 2;
   int32_t _workers = std::max(1, static_cast<int32_t>(std::thread::hardware_concurrency()) - 2);
   uint32_t _seed = 1;
   float _sigma = 0.3f;
   float _rate = 0.5f;
   double _kill_weight = 3.0;
   bool _relative = false;
   std::string _start;
   std::string _out = "learnedbot.txt";
   std::string _work = "sim_train";
};

//! what one kind of bot achieved in a match, per bot and round
struct Result
{
   double _reward = 0.0;
   double _kills = 0.0;
   double _deaths = 0.0;
   double _wins = 0.0;
   double _extras = 0.0;
   int32_t _rows = 0;
};

struct MatchResult
{
   Result _learned;
   Result _proto;
};

using Parameters = std::array<float, LearnedBotWeights::ParameterCount>;

template <typename T>
bool parseNumber(std::string_view text, T& value)
{
   const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
   return error == std::errc{} && end == text.data() + text.size();
}

std::optional<Options> parseOptions(int argc, char** argv)
{
   Options options;

   // argv[1] is --train
   for (int i = 2; i < argc; i += 2)
   {
      if (i + 1 >= argc)
      {
         return std::nullopt;
      }

      const std::string_view name = argv[i];
      const std::string_view value = argv[i + 1];
      bool valid = true;

      if (name == "--generations")
         valid = parseNumber(value, options._generations);
      else if (name == "--population")
         valid = parseNumber(value, options._population);
      else if (name == "--rounds")
         valid = parseNumber(value, options._rounds);
      else if (name == "--duration")
         valid = parseNumber(value, options._duration_s);
      else if (name == "--bots")
         valid = parseNumber(value, options._bots);
      else if (name == "--learned")
         valid = parseNumber(value, options._learned);
      else if (name == "--workers")
         valid = parseNumber(value, options._workers);
      else if (name == "--seed")
         valid = parseNumber(value, options._seed);
      else if (name == "--sigma")
         valid = parseNumber(value, options._sigma);
      else if (name == "--rate")
         valid = parseNumber(value, options._rate);
      else if (name == "--kill-weight")
         valid = parseNumber(value, options._kill_weight);
      else if (name == "--relative")
         options._relative = value == "1" || value == "yes";
      else if (name == "--start")
         options._start = value;
      else if (name == "--out")
         options._out = value;
      else if (name == "--work")
         options._work = value;
      else
         valid = false;

      if (!valid)
      {
         return std::nullopt;
      }
   }

   const bool valid = options._population >= 2 && options._population % 2 == 0 && options._learned >= 1 &&
                      options._learned <= options._bots && options._workers >= 1 && options._sigma > 0.0f;

   return valid ? std::optional(options) : std::nullopt;
}

void add(Result& result, std::span<const std::string> fields)
{
   // round,player,nick,bot,kills,deaths,wins,extras,survival_s
   double kills = 0.0;
   double deaths = 0.0;
   double wins = 0.0;
   double extras = 0.0;
   parseNumber(fields[4], kills);
   parseNumber(fields[5], deaths);
   parseNumber(fields[6], wins);
   parseNumber(fields[7], extras);

   result._kills += kills;
   result._deaths += deaths;
   result._wins += wins;
   result._extras += extras;
   result._rows++;
}

void average(Result& result)
{
   if (result._rows == 0)
   {
      return;
   }

   const double rows = static_cast<double>(result._rows);
   result._kills /= rows;
   result._deaths /= rows;
   result._wins /= rows;
   result._extras /= rows;
   result._reward = 3.0 * result._kills - 2.0 * result._deaths + 2.0 * result._wins + 0.2 * result._extras;
}

//! what training maximizes: the learned bots' score with the chosen kill weight, optionally
//! minus the ProtoBots' score in the same match so luck that hits everyone cancels out
double fitness(const MatchResult& match, const Options& options)
{
   const auto score = [&options](const Result& result)
   { return options._kill_weight * result._kills - 2.0 * result._deaths + 2.0 * result._wins + 0.2 * result._extras; };

   const bool compare = options._relative && match._proto._rows > 0;
   return score(match._learned) - (compare ? score(match._proto) : 0.0);
}

std::optional<MatchResult> readMatch(const std::filesystem::path& csv)
{
   std::ifstream file(csv);

   if (!file.is_open())
   {
      return std::nullopt;
   }

   MatchResult match;
   std::string line;
   std::getline(file, line);

   while (std::getline(file, line))
   {
      std::vector<std::string> fields;
      std::istringstream cells(line);
      std::string cell;

      while (std::getline(cells, cell, ','))
      {
         fields.push_back(cell);
      }

      if (fields.size() < 9)
      {
         continue;
      }

      add(fields[3] == "learned" ? match._learned : match._proto, fields);
   }

   average(match._learned);
   average(match._proto);

   return match._learned._rows > 0 ? std::optional(match) : std::nullopt;
}

class Runner
{
public:
   Runner(const Options& options, std::filesystem::path exe) : _options(options), _exe(std::move(exe))
   {
   }

   //! plays one match per parameter set, all on the same seed, several at once
   std::vector<std::optional<MatchResult>> play(const std::vector<Parameters>& candidates, uint32_t seed, int32_t rounds, std::string_view tag)
   {
      std::vector<std::optional<MatchResult>> results(candidates.size());
      std::atomic<size_t> next = 0;

      const auto work = [&]()
      {
         for (size_t index = next++; index < candidates.size(); index = next++)
         {
            results[index] = playOne(candidates[index], seed, rounds, std::format("{}_{}", tag, index));
         }
      };

      {
         std::vector<std::jthread> threads;
         const size_t count = std::min(candidates.size(), static_cast<size_t>(_options._workers));

         for (size_t i = 0; i < count; i++)
         {
            threads.emplace_back(work);
         }
      }

      return results;
   }

private:
   std::optional<MatchResult> playOne(const Parameters& parameters, uint32_t seed, int32_t rounds, const std::string& name)
   {
      const std::filesystem::path weights_path = std::filesystem::path(_options._work) / (name + ".txt");
      const std::filesystem::path csv_path = std::filesystem::path(_options._work) / (name + ".csv");

      std::filesystem::remove(csv_path);

      if (!LearnedBotWeights::fromParameters(parameters).write(weights_path.string()))
      {
         return std::nullopt;
      }

      const std::string command = std::format(
         "\"\"{}\" --rounds {} --duration {} --bots {} --learned {} --weights \"{}\" --seed {} --out \"{}\" >NUL 2>&1\"",
         _exe.string(),
         rounds,
         _options._duration_s,
         _options._bots,
         _options._learned,
         weights_path.string(),
         seed,
         csv_path.string()
      );

      if (std::system(command.c_str()) != 0)
      {
         return std::nullopt;
      }

      return readMatch(csv_path);
   }

   const Options& _options;
   std::filesystem::path _exe;
};

std::filesystem::path ownExecutable(const char* argv0)
{
   std::filesystem::path path = std::filesystem::absolute(argv0);

   if (!path.has_extension())
   {
      path += ".exe";
   }

   return path;
}

std::string describe(const Result& result)
{
   return std::format(
      "score {:+.3f}  kills {:.3f}  deaths {:.3f}  wins {:.3f}  extras {:.2f}", result._reward, result._kills, result._deaths, result._wins, result._extras
   );
}

}  // namespace

int Trainer::run(int argc, char** argv)
{
   const auto parsed = parseOptions(argc, argv);

   if (!parsed)
   {
      std::cerr << "usage: dynablaster_sim --train [--generations n] [--population even n] [--rounds n] [--duration s] [--bots n] "
                   "[--learned n] [--sigma f] [--rate f] [--workers n] [--start file] [--out file] [--work dir] [--seed n]\n";
      return 2;
   }

   const Options& options = *parsed;
   std::filesystem::create_directories(options._work);

   LearnedBotWeights start = LearnedBotWeights::defaults();

   if (!options._start.empty())
   {
      const auto loaded = LearnedBotWeights::load(options._start);

      if (!loaded)
      {
         std::cerr << std::format("can't read weights from {}\n", options._start);
         return 1;
      }

      start = *loaded;
   }

   Parameters center = start.toParameters();
   Runner runner(options, ownExecutable(argv[0]));
   std::mt19937 random(options._seed);
   std::normal_distribution<float> normal(0.0f, 1.0f);

   // the weights as they are, on seeds training never sees, against ProtoBots
   const auto evaluate = [&](int32_t generation)
   {
      constexpr uint32_t evaluation_seed = 900'000;
      const auto results = runner.play({center}, evaluation_seed, options._rounds * 4, "eval");

      if (!results[0])
      {
         std::cout << std::format("gen {:>3}  evaluation failed\n", generation);
         return;
      }

      std::cout << std::format("gen {:>3}  learned {}\n", generation, describe(results[0]->_learned));
      std::cout << std::format("         proto   {}\n", describe(results[0]->_proto));
      std::cout.flush();

      // keep every evaluated state, e.g. as difficulty levels
      std::filesystem::path snapshot = options._out;
      snapshot.replace_extension(std::format(".g{:03}{}", generation, std::filesystem::path(options._out).extension().string()));
      LearnedBotWeights::fromParameters(center).write(snapshot.string());
   };

   evaluate(0);

   for (int32_t generation = 1; generation <= options._generations; generation++)
   {
      const int32_t pairs = options._population / 2;
      std::vector<Parameters> noise(static_cast<size_t>(pairs));
      std::vector<Parameters> candidates;

      for (Parameters& epsilon : noise)
      {
         Parameters plus = center;
         Parameters minus = center;

         for (size_t i = 0; i < epsilon.size(); i++)
         {
            epsilon[i] = normal(random);
            plus[i] += options._sigma * epsilon[i];
            minus[i] -= options._sigma * epsilon[i];
         }

         candidates.push_back(plus);
         candidates.push_back(minus);
      }

      const uint32_t seed = options._seed * 1000 + static_cast<uint32_t>(generation);
      const auto results = runner.play(candidates, seed, options._rounds, "cand");

      // centered ranks, a failed match ranks last
      std::vector<double> rewards(candidates.size());

      for (size_t i = 0; i < results.size(); i++)
      {
         rewards[i] = results[i] ? fitness(*results[i], options) : -1e9;
      }

      std::vector<size_t> order(candidates.size());
      std::iota(order.begin(), order.end(), 0);
      std::ranges::sort(order, [&rewards](size_t a, size_t b) { return rewards[a] < rewards[b]; });

      std::vector<float> ranks(candidates.size());

      for (size_t rank = 0; rank < order.size(); rank++)
      {
         ranks[order[rank]] = static_cast<float>(rank) / static_cast<float>(order.size() - 1) - 0.5f;
      }

      const float step = options._rate / (static_cast<float>(candidates.size()) * options._sigma);

      for (size_t pair = 0; pair < noise.size(); pair++)
      {
         const float difference = ranks[2 * pair] - ranks[2 * pair + 1];

         for (size_t i = 0; i < center.size(); i++)
         {
            center[i] += step * difference * noise[pair][i];
         }
      }

      const auto failed = std::ranges::count_if(results, [](const auto& result) { return !result.has_value(); });
      const double mean = std::accumulate(rewards.begin(), rewards.end(), 0.0) / static_cast<double>(rewards.size());

      std::cout << std::format(
         "gen {:>3}  candidates: mean fitness {:+.3f}  best {:+.3f}{}\n",
         generation,
         failed > 0 ? 0.0 : mean,
         rewards[order.back()],
         failed > 0 ? std::format("  ({} matches failed)", failed) : std::string()
      );
      std::cout.flush();

      LearnedBotWeights::fromParameters(center).write(options._out);

      if (generation % 5 == 0 || generation == options._generations)
      {
         evaluate(generation);
      }
   }

   std::cout << std::format("weights in {}\n", options._out);

   for (int i = 0; i < LearnedBotWeights::ParameterCount; i++)
   {
      std::cout << std::format("  {:<20} {:+.3f}\n", LearnedBotWeights::parameterName(i), center[static_cast<size_t>(i)]);
   }

   return 0;
}

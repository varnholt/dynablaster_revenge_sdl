#include "hosthistory.h"

#include "stringutils.h"

#include <algorithm>
#include <fstream>

namespace
{
#ifdef __SWITCH__
const std::string HISTORY_FILE = "sdmc:/switch/dynablaster_revenge/history.dr";
#else
const std::string HISTORY_FILE = "history.dr";
#endif
constexpr size_t HISTORY_MAX_ENTRIES = 3;
}  // namespace

void HostHistory::add(const std::string& host)
{
   if (host.empty())
   {
      return;
   }

   deserialize();

   std::vector<std::string> hosts;
   hosts.push_back(host);

   // add all entries from the original host list (skip duplicates, do not exceed max size)
   for (const std::string& previous_host : _hosts)
   {
      if (previous_host != host && hosts.size() < HISTORY_MAX_ENTRIES)
      {
         hosts.push_back(previous_host);
      }
   }

   _hosts = hosts;

   // write entries to disk
   serialize();
}

std::vector<std::string> HostHistory::load(const std::string& selected)
{
   deserialize();
   std::vector<std::string> hosts = _hosts;

   std::erase(hosts, selected);
   hosts.insert(hosts.begin(), selected);

   return hosts;
}

void HostHistory::serialize()
{
   std::ofstream file(HISTORY_FILE);
   if (file.is_open())
   {
      for (const std::string& host : _hosts)
      {
         file << host << "\n";
      }
   }
}

void HostHistory::deserialize()
{
   _hosts.clear();

   std::ifstream file(HISTORY_FILE);
   if (!file.is_open())
   {
      return;
   }

   std::string line;
   while (std::getline(file, line))
   {
      if (std::ranges::find(_hosts, line) == _hosts.end() && !StringUtils::trim(line).empty() && _hosts.size() < HISTORY_MAX_ENTRIES)
      {
         _hosts.push_back(line);
      }
   }
}

#include "datapaths.h"

#include <ranges>
#include <system_error>
#include <vector>

namespace
{
std::vector<std::string> path_list;
}  // namespace

void DataPaths::add(const std::string& path)
{
   path_list.push_back(path);
}

void DataPaths::remove(const std::string& path)
{
   std::erase(path_list, path);
}

std::optional<std::filesystem::path> DataPaths::resolve(const std::string& name)
{
   std::error_code error;
   for (const auto& directory : path_list | std::views::reverse)
   {
      std::filesystem::path candidate(directory + "/" + name);
      if (std::filesystem::exists(candidate, error))
      {
         return candidate;
      }
   }

   std::filesystem::path candidate(name);
   if (std::filesystem::exists(candidate, error))
   {
      return candidate;
   }

   return std::nullopt;
}

std::ifstream DataPaths::open(const std::string& name)
{
   std::ifstream file;
   if (const auto path = resolve(name))
   {
      file.open(*path, std::ios::binary);
   }
   return file;
}

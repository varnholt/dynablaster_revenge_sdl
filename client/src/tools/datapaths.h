// search paths for data files
// a name is looked up in the most recently added directory first, then as given

#pragma once

#include <filesystem>
#include <fstream>
#include <optional>
#include <string>

class DataPaths
{
public:
   static void add(const std::string& path);
   static void remove(const std::string& path);

   static std::optional<std::filesystem::path> resolve(const std::string& name);

   // binary input stream of the resolved file, not open if there is none
   static std::ifstream open(const std::string& name);
};

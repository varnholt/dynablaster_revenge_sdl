#pragma once

#include <string>
#include <vector>

class HostHistory
{
public:
   //! add an entry
   void add(const std::string& host);

   //! get all entries, the selected one first
   std::vector<std::string> load(const std::string& selected = std::string());

protected:
   //! serialize entries
   void serialize();

   //! deserialize entries
   void deserialize();

   //! list of hosts
   std::vector<std::string> _hosts;
};

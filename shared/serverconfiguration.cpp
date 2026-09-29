#include "serverconfiguration.h"

#include "binaryreader.h"
#include "binarywriter.h"

void ServerConfiguration::setBombTickTime(int32_t time)
{
   _bomb_tick_time = time;
}

int32_t ServerConfiguration::getBombTickTime() const
{
   return _bomb_tick_time;
}

BinaryWriter& operator<<(BinaryWriter& out, const ServerConfiguration& config)
{
   out << config.getBombTickTime();
   return out;
}

BinaryReader& operator>>(BinaryReader& in, ServerConfiguration& config)
{
   int32_t bomb_tick_time = 0;
   in >> bomb_tick_time;
   config.setBombTickTime(bomb_tick_time);
   return in;
}

#pragma once

#include <cstdint>

class BinaryWriter;
class BinaryReader;

class ServerConfiguration
{
public:
   ServerConfiguration() = default;

   void setBombTickTime(int32_t time);
   [[nodiscard]] int32_t getBombTickTime() const;

protected:
   int32_t _bomb_tick_time = 0;
};

BinaryWriter& operator<<(BinaryWriter& out, const ServerConfiguration& config);
BinaryReader& operator>>(BinaryReader& in, ServerConfiguration& config);

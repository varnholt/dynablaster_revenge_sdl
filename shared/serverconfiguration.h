#pragma once

#include <cstdint>

class BinaryWriter;
class BinaryReader;

class ServerConfiguration
{
public:
   //! constructor
   ServerConfiguration();

   //! setter for bomb tick time
   void setBombTickTime(int32_t time);

   //! getter for bomb tick time
   [[nodiscard]] int32_t getBombTickTime() const;

protected:
   //! bomb tick time
   int32_t _bomb_tick_time;
};

BinaryWriter& operator<<(BinaryWriter& out, const ServerConfiguration& config);
BinaryReader& operator>>(BinaryReader& in, ServerConfiguration& config);

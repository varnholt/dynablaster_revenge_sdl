// header
#include "serverconfiguration.h"

// shared
#include "binaryreader.h"
#include "binarywriter.h"

//-----------------------------------------------------------------------------
/*!
 */
ServerConfiguration::ServerConfiguration() : _bomb_tick_time(0)
{
}

//-----------------------------------------------------------------------------
/*!
   \param time bomb tick time
*/
void ServerConfiguration::setBombTickTime(int32_t time)
{
   _bomb_tick_time = time;
}

//-----------------------------------------------------------------------------
/*!
   \return bomb tick time
*/
int32_t ServerConfiguration::getBombTickTime() const
{
   return _bomb_tick_time;
}

//-----------------------------------------------------------------------------
/*!
   \param out datastream out
   \param config server configuration reference
*/
BinaryWriter& operator<<(BinaryWriter& out, const ServerConfiguration& config)
{
   out << config.getBombTickTime();
   return out;
}

//-----------------------------------------------------------------------------
/*!
   \param in datastream in
   \param config server configuration reference
*/
BinaryReader& operator>>(BinaryReader& in, ServerConfiguration& config)
{
   int32_t bomb_tick_time = 0;

   in >> bomb_tick_time;

   config.setBombTickTime(bomb_tick_time);

   return in;
}

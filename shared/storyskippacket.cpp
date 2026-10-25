#include "storyskippacket.h"

#include "log.h"

#include <string_view>

namespace
{
constexpr std::string_view PACKETNAME = "StorySkip";
}

StorySkipPacket::StorySkipPacket() : Packet(Packet::STORYSKIP)
{
   _packet_name = PACKETNAME;
}

void StorySkipPacket::enqueue(BinaryWriter& /*out*/)
{
}

void StorySkipPacket::dequeue(BinaryReader& /*in*/)
{
}

void StorySkipPacket::debug()
{
   Log::info("StorySkipPacket");
}

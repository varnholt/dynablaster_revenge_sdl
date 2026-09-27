#include "packet.h"

#include "logging.h"

#include <chrono>

// shared
#include "bombpacket.h"
#include "countdownpacket.h"
#include "creategamerequestpacket.h"
#include "creategameresponsepacket.h"
#include "detonationpacket.h"
#include "errorpacket.h"
#include "extramapitemcreatedpacket.h"
#include "extrashakepacket.h"
#include "gameeventpacket.h"
#include "gamestatspacket.h"
#include "joingamerequestpacket.h"
#include "joingameresponsepacket.h"
#include "keypacket.h"
#include "leavegamerequestpacket.h"
#include "leavegameresponsepacket.h"
#include "listgamesrequestpacket.h"
#include "listgamesresponsepacket.h"
#include "loginrequestpacket.h"
#include "loginresponsepacket.h"
#include "mapcreaterequestpacket.h"
#include "mapitemcreatedpacket.h"
#include "mapitemdestroyedpacket.h"
#include "mapitemmovepacket.h"
#include "mapitemremovedpacket.h"
#include "messagepacket.h"
#include "playerinfectedpacket.h"
#include "playerkilledpacket.h"
#include "playermodifiedpacket.h"
#include "playersynchronizepacket.h"
#include "positionpacket.h"
#include "startgamerequestpacket.h"
#include "startgameresponsepacket.h"
#include "stonedroppacket.h"
#include "stopgamerequestpacket.h"
#include "stopgameresponsepacket.h"
#include "timepacket.h"

namespace
{
int32_t currentMsecsSinceMidnight()
{
   using namespace std::chrono;
   const auto now = system_clock::now();
   const auto midnight = floor<std::chrono::days>(now);
   return static_cast<int32_t>(duration_cast<milliseconds>(now - midnight).count());
}
}  // namespace

//-----------------------------------------------------------------------------
/*!
   read constructor
*/
Packet::Packet() : mPacketSize(0), mPacketType(INVALID), mPacketName("INVALID")
{
   mTimestamp = currentMsecsSinceMidnight();
}

//-----------------------------------------------------------------------------
/*!
   write constructor
*/
Packet::Packet(TYPE type) : mPacketSize(0), mPacketType(type)
{
   mTimestamp = currentMsecsSinceMidnight();
}

//-----------------------------------------------------------------------------
/*!
   destructor
*/
Packet::~Packet()
{
}

//-----------------------------------------------------------------------------
/*!
   \return packet size
*/
int16_t Packet::getSize()
{
   return mPacketSize;
}

//-----------------------------------------------------------------------------
/*!
   \return packet type
*/
Packet::TYPE Packet::getType()
{
   return mPacketType;
}

//-----------------------------------------------------------------------------
/*!
   \return packet timestamp
*/
int32_t Packet::getTimestamp() const
{
   return mTimestamp;
}

//-----------------------------------------------------------------------------
/*!
   \param time new timestamp
*/
void Packet::setTimeStamp(int32_t time)
{
   mTimestamp = time;
}

//-----------------------------------------------------------------------------
/*!
   \return packet name
*/
const std::string& Packet::getPacketName() const
{
   return mPacketName;
}

//-----------------------------------------------------------------------------
/*!
   \return raw pointer to the serialized packet bytes
*/
const char* Packet::constData() const
{
   return reinterpret_cast<const char*>(data());
}

//-----------------------------------------------------------------------------
/*!
   serialize a packet
*/
void Packet::serialize()
{
   // Packet is itself the byte buffer (derives from std::vector<uint8_t>) and BinaryWriter only
   // ever appends - clear it first so calling serialize() more than once on the same instance
   // re-produces the same bytes instead of duplicating the payload onto itself.
   clear();

   BinaryWriter out(*this);

   const auto sizeOffset = out.pos();

   // reserve 16 bits for the packet size
   out << static_cast<uint16_t>(0);

   // write packet packetType
   out << static_cast<uint8_t>(mPacketType);

   // write timestamp to packet
   out << mTimestamp;

   enqueue(out);

   // patch in the blocksize now that the payload's length is known
   out.patchUint16(sizeOffset, static_cast<uint16_t>(size() - sizeOffset - sizeof(uint16_t)));
}

/*!----------------------------------------------------------------------------
   deserialize a packet

   \param in input reader, positioned right after the packet's size prefix
   \return a packet of the correct type with all member variables filled
*/
std::unique_ptr<Packet> Packet::deserialize(BinaryReader& in)
{
   int8_t pType;

   // read the serialized data
   in >> pType;

   std::unique_ptr<Packet> packet;

   switch (pType)
   {
      case Packet::BOMB:
         packet = std::make_unique<BombPacket>();
         break;
      case Packet::COUNTDOWN:
         packet = std::make_unique<CountdownPacket>();
         break;
      case Packet::CREATEGAMEREQUEST:
         packet = std::make_unique<CreateGameRequestPacket>();
         break;
      case Packet::CREATEGAMERESPONSE:
         packet = std::make_unique<CreateGameResponsePacket>();
         break;
      case Packet::DETONATION:
         packet = std::make_unique<DetonationPacket>();
         break;
      case Packet::EXTRAMAPITEMCREATED:
         packet = std::make_unique<ExtraMapItemCreatedPacket>();
         break;
      case Packet::EXTRASHAKE:
         packet = std::make_unique<ExtraShakePacket>();
         break;
      case Packet::GAMEEVENT:
         packet = std::make_unique<GameEventPacket>();
         break;
      case Packet::GAMESTATS:
         packet = std::make_unique<GameStatsPacket>();
         break;
      case Packet::JOINGAMEREQUEST:
         packet = std::make_unique<JoinGameRequestPacket>();
         break;
      case Packet::JOINGAMERESPONSE:
         packet = std::make_unique<JoinGameResponsePacket>();
         break;
      case Packet::KEY:
         packet = std::make_unique<KeyPacket>();
         break;
      case Packet::LEAVEGAMEREQUEST:
         packet = std::make_unique<LeaveGameRequestPacket>();
         break;
      case Packet::LEAVEGAMERESPONSE:
         packet = std::make_unique<LeaveGameResponsePacket>();
         break;
      case Packet::LISTGAMESREQUEST:
         packet = std::make_unique<ListGamesRequestPacket>();
         break;
      case Packet::LISTGAMESRESPONSE:
         packet = std::make_unique<ListGamesResponsePacket>();
         break;
      case Packet::LOGINREQUEST:
         packet = std::make_unique<LoginRequestPacket>();
         break;
      case Packet::LOGINRESPONSE:
         packet = std::make_unique<LoginResponsePacket>();
         break;
      case Packet::MAPCREATEREQUEST:
         packet = std::make_unique<MapCreateRequestPacket>();
         break;
      case Packet::MAPITEMCREATED:
         packet = std::make_unique<MapItemCreatedPacket>();
         break;
      case Packet::MAPITEMREMOVED:
         packet = std::make_unique<MapItemRemovedPacket>();
         break;
      case Packet::MAPITEMDESTROYED:
         packet = std::make_unique<MapItemDestroyedPacket>();
         break;
      case Packet::MESSAGE:
         packet = std::make_unique<MessagePacket>();
         break;
      case Packet::MAPITEMMOVE:
         packet = std::make_unique<MapItemMovePacket>();
         break;
      case Packet::PLAYERKILLED:
         packet = std::make_unique<PlayerKilledPacket>();
         break;
      case Packet::PLAYERMODIFIED:
         packet = std::make_unique<PlayerModifiedPacket>();
         break;
      case Packet::POSITION:
         packet = std::make_unique<PositionPacket>();
         break;
      case Packet::STARTGAMEREQUEST:
         packet = std::make_unique<StartGameRequestPacket>();
         break;
      case Packet::STARTGAMERESPONSE:
         packet = std::make_unique<StartGameResponsePacket>();
         break;
      case Packet::STONEDROP:
         packet = std::make_unique<StoneDropPacket>();
         break;
      case Packet::STOPGAMEREQUEST:
         packet = std::make_unique<StopGameRequestPacket>();
         break;
      case Packet::STOPGAMERESPONSE:
         packet = std::make_unique<StopGameResponsePacket>();
         break;
      case Packet::TIME:
         packet = std::make_unique<TimePacket>();
         break;
      case Packet::PLAYERSYNCHRONIZEPACKET:
         packet = std::make_unique<PlayerSynchronizePacket>();
         break;
      case Packet::PLAYERINFECTEDPACKET:
         packet = std::make_unique<PlayerInfectedPacket>();
         break;
      case Packet::ERROR:
         packet = std::make_unique<ErrorPacket>();
         break;
      default:
         break;
   }

   if (packet)
   {
      // read timestamp
      in >> packet->mTimestamp;

      // dequeue members
      packet->dequeue(in);
   }
   else
   {
      qWarning("unknown packet received");
   }

   return packet;
}

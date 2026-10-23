#include "packet.h"

#include "logging.h"

#include <chrono>

// shared
#include "bombpacket.h"
#include "countdownpacket.h"
#include "creategamerequestpacket.h"
#include "creategameresponsepacket.h"
#include "detonationpacket.h"
#include "enemycreatedpacket.h"
#include "enemyhitpacket.h"
#include "enemykilledpacket.h"
#include "enemypositionpacket.h"
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
#include "storystatepacket.h"
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

Packet::Packet() : _timestamp(currentMsecsSinceMidnight()), _packet_name("INVALID")
{
}

Packet::Packet(TYPE packet_type) : _packet_type(packet_type), _timestamp(currentMsecsSinceMidnight())
{
}

int16_t Packet::getSize() const
{
   return _packet_size;
}

Packet::TYPE Packet::getType() const
{
   return _packet_type;
}

int32_t Packet::getTimestamp() const
{
   return _timestamp;
}

void Packet::setTimeStamp(int32_t time)
{
   _timestamp = time;
}

const std::string& Packet::getPacketName() const
{
   return _packet_name;
}

void Packet::serialize()
{
   // BinaryWriter only appends - clear first so repeated serialize() calls produce the same bytes
   clear();

   BinaryWriter out(*this);

   const auto size_offset = out.pos();

   // reserve 16 bits for the packet size
   out << static_cast<uint16_t>(0);
   out << static_cast<uint8_t>(_packet_type);
   out << _timestamp;

   enqueue(out);

   // patch in the block size now that the payload's length is known
   out.patchUint16(size_offset, static_cast<uint16_t>(size() - size_offset - sizeof(uint16_t)));
}

std::unique_ptr<Packet> Packet::deserialize(BinaryReader& in)
{
   int8_t packet_type = 0;
   in >> packet_type;

   std::unique_ptr<Packet> packet;

   switch (packet_type)
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
      case Packet::ENEMYCREATED:
         packet = std::make_unique<EnemyCreatedPacket>();
         break;
      case Packet::ENEMYPOSITION:
         packet = std::make_unique<EnemyPositionPacket>();
         break;
      case Packet::ENEMYKILLED:
         packet = std::make_unique<EnemyKilledPacket>();
         break;
      case Packet::ENEMYHIT:
         packet = std::make_unique<EnemyHitPacket>();
         break;
      case Packet::STORYSTATE:
         packet = std::make_unique<StoryStatePacket>();
         break;
      default:
         break;
   }

   if (packet)
   {
      in >> packet->_timestamp;
      packet->dequeue(in);
   }
   else
   {
      qWarning("unknown packet received");
   }

   return packet;
}

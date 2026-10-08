#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "binaryreader.h"
#include "binarywriter.h"

// TODO: public inheritance from std::vector<uint8_t> is a design smell (non-polymorphic base,
// no protection against slicing via the container interface) and TYPE below should really be an
// enum class - both are deferred: TYPE alone has ~226 unqualified call sites (Packet::BOMB etc.)
// across client/, server/ and ai/, well outside shared/'s scope. Revisit in the client/server pass.
class Packet : public std::vector<uint8_t>
{
public:
   enum TYPE
   {
      INVALID,
      BOMB,
      COUNTDOWN,
      CREATEGAMEREQUEST,
      CREATEGAMERESPONSE,
      DETONATION,
      ERROR,
      EXTRAMAPITEMCREATED,
      GAMEEVENT,
      GAMESTATS,
      JOINGAMEREQUEST,
      JOINGAMERESPONSE,
      KEY,
      LEAVEGAMEREQUEST,
      LEAVEGAMERESPONSE,
      LISTGAMESREQUEST,
      LISTGAMESRESPONSE,
      LOGINREQUEST,
      LOGINRESPONSE,
      MAPCREATEREQUEST,
      MAPITEMCREATED,
      MAPITEMDESTROYED,
      MAPITEMREMOVED,
      MESSAGE,
      MAPITEMMOVE,
      PLAYERKILLED,
      PLAYERMODIFIED,
      POSITION,
      STARTGAMEREQUEST,
      STARTGAMERESPONSE,
      STOPGAMEREQUEST,
      STOPGAMERESPONSE,
      TIME,
      STONEDROP,
      EXTRASHAKE,
      PLAYERSYNCHRONIZEPACKET,
      PLAYERINFECTEDPACKET,
      ENEMYCREATED,
      ENEMYPOSITION,
      ENEMYKILLED,
      ENEMYHIT,
      STORYSTATE
   };

   // read constructor
   Packet();

   // write constructor
   explicit Packet(TYPE packet_type);

   virtual ~Packet() = default;

   void serialize();

   // reader must be positioned right after the packet's size prefix
   static std::unique_ptr<Packet> deserialize(BinaryReader& in);

   virtual void debug() = 0;
   virtual void enqueue(BinaryWriter& out) = 0;
   virtual void dequeue(BinaryReader& in) = 0;

   [[nodiscard]] int16_t getSize() const;
   [[nodiscard]] TYPE getType() const;

   // milliseconds since midnight
   [[nodiscard]] int32_t getTimestamp() const;

   // timestamp can be modified (e.g. in playback)
   void setTimeStamp(int32_t time);

   [[nodiscard]] const std::string& getPacketName() const;

protected:
   int16_t _packet_size = 0;
   TYPE _packet_type = INVALID;

   // milliseconds since midnight
   int32_t _timestamp = 0;

   std::string _packet_name;
};

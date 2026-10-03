#pragma once

#include "constants.h"
#include "elapsedtimer.h"
#include "mapitem.h"

#include <vector>

class ExtraMapItemCreatedPacket;

class ExtraMapItem : public MapItem
{
public:
   ExtraMapItem(int32_t id, Constants::ExtraType type, int32_t x, int32_t y);

   explicit ExtraMapItem(const ExtraMapItemCreatedPacket& packet);

   [[nodiscard]] Constants::ExtraType getExtraType() const;

   void initializeStartTime();

   // seconds since the extra became visible
   [[nodiscard]] float getElapsedTime() const;

   void setSkullFaces(const std::vector<Constants::SkullType>& faces);
   [[nodiscard]] std::vector<Constants::SkullType> getSkullFaces() const;

private:
   Constants::ExtraType _extra_type;
   ElapsedTimer _start_time;
   std::vector<Constants::SkullType> _skull_faces;
};

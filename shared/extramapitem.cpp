#include "extramapitem.h"

#include "extramapitemcreatedpacket.h"

ExtraMapItem::ExtraMapItem(int32_t id, Constants::ExtraType type, int32_t x, int32_t y)
    : MapItem(Extra, id, false, true, x, y), _extra_type(type)
{
}

ExtraMapItem::ExtraMapItem(ExtraMapItemCreatedPacket* packet)
    : MapItem(packet), _extra_type(static_cast<Constants::ExtraType>(packet->getExtraType()))
{
}

Constants::ExtraType ExtraMapItem::getExtraType() const
{
   return _extra_type;
}

void ExtraMapItem::initializeStartTime()
{
   _start_time.start();
}

float ExtraMapItem::getElapsedTime() const
{
   return static_cast<float>(_start_time.elapsed()) * 0.001f;
}

void ExtraMapItem::setSkullFaces(const std::vector<Constants::SkullType>& faces)
{
   _skull_faces = faces;
}

std::vector<Constants::SkullType> ExtraMapItem::getSkullFaces() const
{
   return _skull_faces;
}

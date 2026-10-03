// light object
// all lights have a colortrack and an exclude list (list of object-ids which are not affected by a light)

#pragma once

#include <cstdint>
#include <vector>
#include "animation/postrack.h"
#include "node.h"

class Stream;

class Light : public Node
{
public:
   explicit Light(Node::ID id);
   Vector getColor() const;
   void transform(float frame) override;
   void load(Stream& stream) override;
   void write(Stream& stream) override;

protected:
   int32_t _flags = 0;
   float _attenuation_start = 0.0f;
   float _attenuation_end = 0.0f;
   Vector _color;
   PosTrack _color_track;
   std::vector<int32_t> _exclude;
};

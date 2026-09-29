#ifndef EXTRA_H
#define EXTRA_H

#include "nodes/mesh.h"
#include "math/matrix.h"

// shared
#include "constants.h"
#include "extramapitem.h"

class Extra : public Mesh
{
public:
   Extra(Constants::ExtraType type, Geometry *geo, float x, float y);
   ~Extra() override;
   void animate(float time);

private:
   Constants::ExtraType    _type;
   Matrix                  _position;
   float                   _time_offset;
};

#endif

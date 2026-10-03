#include "extra.h"

#include <cmath>
#include <numbers>

#ifdef Q_OS_MAC
#include "stdlib.h"
#endif

Extra::Extra(Constants::ExtraType type, Geometry* object, float x, float y) : Mesh(), _type(type)
{
   Geometry* geo = new Geometry(this);
   geo->copy(*object);
   add(geo);
   setUserTransformable(true);

   _position.identity();
   _position.translate(Vector(x + 0.5, -y - 0.5f, 0.0f));
   setTransform(_position);

   _time_offset = (rand() & 4095) * std::numbers::pi_v<float> / 2048.0f;
}

Extra::~Extra()
{
}

void Extra::animate(float time)
{
   float a = (float)std::sin(_time_offset + time * 0.04) * 0.9f;
   float s = (float)std::sin(_time_offset + time * 0.2) * 0.2f;

   Matrix scale = Matrix::scale(0.9f - s, 1.0f, 0.9f + s);
   Matrix rot = Matrix::rotateZ(a);
   setTransform(scale * rot * _position);
}

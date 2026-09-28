// header
#include "skull.h"

// framework
#include "globaltime.h"

Skull::Skull(Mesh *reference, float x, float y)
 : Mesh(*reference),
   _reference(reference)
{
   setUserTransformable(true);

   _translation.identity();
   _translation.translate( Vector(x + 0.5f, -y - 0.5f, 0.5f) );

   // animation starts at 0
   setAnimationFrame(0.0f);

   // animation start time
   _start_time = GlobalTime::Instance()->getTime();
}


Skull::~Skull()
{
}


Mesh *Skull::getReference() const
{
   return _reference;
}


Matrix Skull::getTranslation()  const
{
   return _translation;
}


float Skull::getStartTime() const
{
   return _start_time;
}

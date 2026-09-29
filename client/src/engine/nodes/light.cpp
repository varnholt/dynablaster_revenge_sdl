#include "light.h"

Light::Light(Node::ID id, Node* parent) : Node(id, parent)
{
}

void Light::transform(float time)
{
   Node::transform(time);
   _color = _color_track.get(time);
}

void Light::load(Stream* stream)
{
   _flags = stream->getInt();

   _attenuation_start = stream->getFloat();
   _attenuation_end = stream->getFloat();

   // load exclude list
   Chunk list(stream);
   const int32_t size = list.getInt();
   _exclude.init(size);
   for (int32_t i = 0; i < size; i++)
   {
      _exclude.add(list.getInt());
   }
   list.skip();

   Chunk animation(&list);
   _color_track.load(&animation);
   _position_track.load(&animation);
   _rotation_track.load(&animation);
   _scale_track.load(&animation);
   _visibility_track.load(&animation);
   _flip_track.load(&animation);
   animation.skip();
}

void Light::write(Stream* stream)
{
   // write exclude list
   {
      Chunk list(stream, 500, "Light");

      list.writeInt(_exclude.size());
      for (int32_t i = 0; i < _exclude.size(); i++)
      {
         list.writeInt(_exclude[i]);
      }
   }

   Chunk animation(stream, 2000, "Animation");
   _color_track.write(&animation);
   _position_track.write(&animation);
   _rotation_track.write(&animation);
   _scale_track.write(&animation);
   _visibility_track.write(&animation);
   _flip_track.write(&animation);
}

Vector Light::getColor() const
{
   return _color;
}

#include "light.h"

Light::Light(Node::ID id) : Node(id)
{
}

void Light::transform(float time)
{
   Node::transform(time);
   _color = _color_track.get(time);
}

void Light::load(Stream& stream)
{
   _flags = stream.getInt();

   _attenuation_start = stream.getFloat();
   _attenuation_end = stream.getFloat();

   // load exclude list
   Chunk list(stream);
   const int32_t size = list.getInt();
   _exclude.clear();
   for (int32_t i = 0; i < size; i++)
   {
      _exclude.push_back(list.getInt());
   }
   list.skip();

   Stream& list_stream = list;
   Chunk animation(list_stream);
   _color_track.load(animation);
   _position_track.load(animation);
   _rotation_track.load(animation);
   _scale_track.load(animation);
   _visibility_track.load(animation);
   _flip_track.load(animation);
   animation.skip();
}

void Light::write(Stream& stream)
{
   // write exclude list
   {
      Chunk list(stream, 500, "Light");

      list.writeInt(static_cast<int32_t>(_exclude.size()));
      for (const auto id : _exclude)
      {
         list.writeInt(id);
      }
   }

   Chunk animation(stream, 2000, "Animation");
   _color_track.write(animation);
   _position_track.write(animation);
   _rotation_track.write(animation);
   _scale_track.write(animation);
   _visibility_track.write(animation);
   _flip_track.write(animation);
}

Vector Light::getColor() const
{
   return _color;
}

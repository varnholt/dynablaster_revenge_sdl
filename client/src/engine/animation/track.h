// template track class
// holds a list of animation keys

#pragma once

#include <cstdint>
#include <cstdio>
#include "tools/chunk.h"
#include "tools/list.h"
#include "tools/stream.h"

template <class KeyClass>
class Track : public List<KeyClass>
{
public:
   enum Type
   {
      idUndefined = 0,
      idValue = 2001,
      idPosition = 2002,
      idRotation = 2003,
      idScale = 2004,
      idVisibility = 2005,
      idVertexMorph = 2010
   };

   Track(Type type, const String& name = String()) : _type(type), _name(name)
   {
      if (_name.isEmpty())
      {
         switch (_type)
         {
            case idValue:
               _name = "Value";
               break;
            case idPosition:
               _name = "Position";
               break;
            case idRotation:
               _name = "Rotation";
               break;
            case idScale:
               _name = "Scale";
               break;
            case idVisibility:
               _name = "Visible";
               break;
            case idVertexMorph:
               _name = "VertexMorph";
               break;
            case idUndefined:
            default:
               _name = "Undefined";
               break;
         }
      }
   }

   void addKey(const KeyClass& key)
   {
      this->add(key);
   }

   int32_t getAnimationLength() const
   {
      const int32_t count = this->size();
      if (count > 0)
      {
         return key(count - 1).time();
      }
      return 0;
   }

   void load(Stream* stream) override
   {
      Chunk track(stream);

      if (track.id() == _type)
      {
         List<KeyClass>::load(&track);
      }
      else
      {
         std::printf("wrong track!\n");
      }

      track.skip();
   }

   void write(Stream* stream) override
   {
      Chunk track(stream, _type, _name);

      List<KeyClass>::write(&track);
   }

   float interpolate(float time)
   {
      while (_current_key > 0 && time < key(_current_key).time())
      {
         _current_key--;
      }
      while (_current_key < this->size() - 1 && time >= key(_current_key + 1).time())
      {
         _current_key++;
      }

      const int32_t length = nextKey().time() - prevKey().time();
      const float progress = time - prevKey().time();

      return progress / length;
   }

   KeyClass& prevKey() const
   {
      return key(_current_key);
   }

   KeyClass& nextKey() const
   {
      return key(_current_key + 1);
   }

   KeyClass& key(int32_t index) const
   {
      return (*this)[index];
   }

protected:
   Type _type;
   String _name;
   int32_t _current_key = 0;
};

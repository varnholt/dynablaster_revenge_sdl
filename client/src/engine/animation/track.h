// template track class
// holds a list of animation keys

#pragma once

#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
#include "tools/chunk.h"
#include "tools/stream.h"
#include "tools/streamable.h"

template <class KeyClass>
class Track : public Streamable
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

   Track(Type type, const std::string& name = std::string()) : _type(type), _name(name)
   {
      if (_name.empty())
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
      _keys.push_back(key);
   }

   int32_t size() const
   {
      return static_cast<int32_t>(_keys.size());
   }

   int32_t getAnimationLength() const
   {
      if (!_keys.empty())
      {
         return _keys.back().time();
      }
      return 0;
   }

   void load(Stream& stream) override
   {
      Chunk track(stream);

      if (track.id() == _type)
      {
         loadList(track, _keys);
      }
      else
      {
         std::printf("wrong track!\n");
      }

      track.skip();
   }

   void write(Stream& stream) override
   {
      Chunk track(stream, _type, _name);

      writeList(track, _keys);
   }

   float interpolate(float time)
   {
      while (_current_key > 0 && time < key(_current_key).time())
      {
         _current_key--;
      }
      while (_current_key < size() - 1 && time >= key(_current_key + 1).time())
      {
         _current_key++;
      }

      const int32_t length = nextKey().time() - prevKey().time();
      const float progress = time - prevKey().time();

      return progress / length;
   }

   KeyClass& prevKey()
   {
      return key(_current_key);
   }

   KeyClass& nextKey()
   {
      return key(_current_key + 1);
   }

   KeyClass& key(int32_t index)
   {
      return _keys[index];
   }

   const KeyClass& key(int32_t index) const
   {
      return _keys[index];
   }

protected:
   Type _type;
   std::string _name;
   int32_t _current_key = 0;
   std::vector<KeyClass> _keys;
};

#pragma once

#include <cstdint>
#include <functional>
#include "math/vector.h"

class Material;

class PlayerInvincibleInstance
{
public:
   explicit PlayerInvincibleInstance(Material& material);
   ~PlayerInvincibleInstance();

   bool update(float dt);
   void remove();
   void setRemove(bool remove);

   Material& getMaterial() const;

   int width() const;
   int height() const;

   uint32_t texture(int id) const;
   void bind(int id);
   void unbind();

   void setCenter(const Vector& center);
   const Vector& getCenter() const;

   void setRect(const Vector& min, const Vector& max);
   const Vector& min2d() const;
   const Vector& max2d() const;

   void setFade(float fade);
   float getFade() const;

private:
   std::reference_wrapper<Material> _material;
   int _width = 256;
   int _height = 256;
   uint32_t _texture[2];
   uint32_t _target[2];

   Vector _min;
   Vector _max;
   Vector _center;
   float _fade = 0.0f;
   bool _remove = false;
};

#pragma once

#include "math/vector.h"

class Material;

class PlayerInvincibleInstance
{
public:
   PlayerInvincibleInstance();
   ~PlayerInvincibleInstance();

   bool update(float dt);
   void remove();
   void setRemove(bool remove);

   void setMaterial(Material* mat);
   Material* getMaterial() const;

   int width() const;
   int height() const;

   unsigned int texture(int id) const;
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
   Material* _material;
   int _width;
   int _height;
   unsigned int _texture[2];
   unsigned int _target[2];

   Vector _min;
   Vector _max;
   Vector _center;
   float _fade;
   bool _remove;
};

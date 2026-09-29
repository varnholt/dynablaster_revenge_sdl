#ifndef PLAYER_H
#define PLAYER_H

#include "math/vector.h"
#include "constants.h"

#include <string>

class Material;
class Mesh;

class PlayerItem
{

public:

   PlayerItem(int id, const std::string& nick, Constants::Color color);
   ~PlayerItem();

   int getID() const;
   Constants::Color getColor() const;
   const std::string& getNick() const;

   void setMaterial(Material *mat);
   Material* getMaterial() const;

   void setMesh(Mesh *mesh);
   Mesh* getMesh() const;

   void setRotation(float rot);
   void setPosition(float x, float y);
   const Vector& getPosition() const;

   void setSpeed(float speed);
   float getSpeed() const;

   void animate(float time, float delta);
   void kill();
   void setKilled(bool is_killed);
   bool isKilled() const;
   void win();
   bool isWinner() const;

   void setFlash(float flash);


private:

   void update();

   int _id;
   Constants::Color _color;
   std::string _nick;
   Mesh *_mesh = nullptr;
   Material *_material = nullptr;
   Vector _pos;
   float _rot = 0;
   float _speed = 0.0f;
   float _anim_blend = 0.0f;
   float _stand_blend = 0.0f;
   bool  _killed = false;
   bool  _win = false;
   bool  _left_foot = false;
   float _flash = 0.0f;
};

#endif

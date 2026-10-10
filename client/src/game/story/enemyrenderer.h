#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>

#include "enemytype.h"
#include "math/matrix.h"

class GltfModel;

//! the story mode enemies as animated glTF models, positions come from the server
class EnemyRenderer
{
public:
   EnemyRenderer();
   ~EnemyRenderer();

   void add(int32_t id, EnemyType type, float x, float y, float angle);
   void setPosition(int32_t id, float x, float y, float angle, float dx, float dy, int32_t flags);
   void hit(int32_t id);

   //! killed enemies fall over before they vanish, removed ones vanish right away
   void kill(int32_t id, bool removed);

   void clear();

   //! seconds
   void animate(float dt);

   //! draws with the camera of the scene that rendered last
   void render();

   //! types drawn with the player model instead of a glTF one
   [[nodiscard]] static bool isBomberman(EnemyType type);

private:
   struct Instance
   {
      EnemyType _type = EnemyType::Ballom;
      float _x = 0.0f;
      float _y = 0.0f;
      float _angle = 0.0f;
      float _target_angle = 0.0f;
      float _vx = 0.0f;
      float _vy = 0.0f;
      float _since_update = 0.0f;
      int32_t _flags = 0;
      float _time = 0.0f;
      float _flash = 0.0f;
      float _dying = -1.0f;
   };

   struct Model
   {
      std::unique_ptr<GltfModel> _model;
      int32_t _idle = -1;
      int32_t _walk = -1;
      int32_t _die = -1;
      float _scale = 1.0f;
      float _lift = 0.0f;
   };

   Model& getModel(EnemyType type);

   std::unordered_map<int32_t, Instance> _instances;
   std::unordered_map<int32_t, Model> _models;
};

#include "enemyrenderer.h"

#include "engine/gltf/gltfmodel.h"

// shared
#include "constants.h"
#include "enemypositionpacket.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <numbers>

namespace
{
constexpr float ENEMY_HEIGHT = 0.85f;
constexpr float BOSS_HEIGHT = 1.35f;
constexpr float MAX_FOOTPRINT = 1.8f;
constexpr float TURN_RATE = 12.0f;
constexpr float SERVER_TICK = 1.0f / static_cast<float>(SERVER_HEARTBEAT_IN_HZ);
constexpr float MAX_EXTRAPOLATION = 0.1f;
constexpr float FLASH_TIME = 0.3f;
constexpr float FALL_TIME = 0.6f;
constexpr float DEATH_TIME = 1.3f;

// models with feet ("leg" joints, see assets/enemies/sdf_rig.py) cover what their feet sweep per walk cycle;
// the others (hopping, floating, spinning, slithering) have nothing on the ground to slide and take a cycle
// per this many tiles, about the pace they had at a typical speed
constexpr float NOMINAL_STRIDE = 1.5f;
constexpr float MIN_STRIDE = 0.05f;

// steps per walk cycle the ground covered is measured at
constexpr int32_t WALK_SAMPLES = 96;

// a bigger jump of the shown position isn't walked
constexpr float MAX_STEP = 1.0f;

// seconds without moving until a blocked enemy stands instead of walking on the spot
constexpr float STAND_DELAY = 0.15f;

float wrapAngle(float angle)
{
   constexpr float two_pi = 2.0f * std::numbers::pi_v<float>;
   angle = std::fmod(angle, two_pi);
   if (angle > std::numbers::pi_v<float>)
   {
      angle -= two_pi;
   }
   if (angle < -std::numbers::pi_v<float>)
   {
      angle += two_pi;
   }
   return angle;
}
}  // namespace

EnemyRenderer::EnemyRenderer() = default;

EnemyRenderer::~EnemyRenderer() = default;

bool EnemyRenderer::isBomberman(EnemyType type)
{
   return type == EnemyType::Henchman || type == EnemyType::ShadowBomber;
}

float EnemyRenderer::getWalkTime(const Model& model, float cycles)
{
   const float whole = std::floor(cycles);
   float share = cycles - whole;

   // the carrying foot sweeps back faster in the middle of a step than at its ends: the cycle's time where
   // the ground covered reaches that share of the stride keeps it on the spot
   if (model._covered.size() > 1)
   {
      const float target = share * model._covered.back();
      const auto upper = std::ranges::upper_bound(model._covered, target);
      const auto next = std::clamp<size_t>(static_cast<size_t>(std::distance(model._covered.begin(), upper)), 1, model._covered.size() - 1);
      const float from = model._covered[next - 1];
      const float to = model._covered[next];
      const float within = to > from ? std::clamp((target - from) / (to - from), 0.0f, 1.0f) : 0.0f;
      share = (static_cast<float>(next - 1) + within) / static_cast<float>(model._covered.size() - 1);
   }

   return (whole + share) * model._model->getAnimationDuration(model._walk);
}

EnemyRenderer::Model& EnemyRenderer::getModel(EnemyType type)
{
   const auto key = static_cast<int32_t>(type);
   auto it = _models.find(key);

   if (it != _models.end())
   {
      return it->second;
   }

   Model& model = _models[key];
   model._model = std::make_unique<GltfModel>(std::format("enemies/{}.glb", getEnemyTypeName(type)));

   if (model._model->isValid())
   {
      model._idle = model._model->findAnimation("idle");
      model._walk = model._model->findAnimation("walk");
      model._die = model._model->findAnimation("die");

      const Vector& minimum = model._model->getMinimum();
      const Vector& maximum = model._model->getMaximum();
      const float height = std::max(maximum.z - minimum.z, 0.01f);
      const float footprint = std::max(maximum.x - minimum.x, maximum.y - minimum.y);

      model._scale = (isBoss(type) ? BOSS_HEIGHT : ENEMY_HEIGHT) / height;
      model._scale = std::min(model._scale, MAX_FOOTPRINT / std::max(footprint, 0.01f));
      model._lift = -minimum.z * model._scale;

      if (model._walk >= 0)
      {
         model._covered = model._model->measureWalk(model._walk, "leg", WALK_SAMPLES);
         for (float& covered : model._covered)
         {
            covered *= model._scale;
         }

         if (model._covered.empty() || model._covered.back() < MIN_STRIDE)
         {
            model._covered.clear();
         }
      }
      model._stride = model._covered.empty() ? NOMINAL_STRIDE : model._covered.back();
   }

   return model;
}

void EnemyRenderer::add(int32_t id, EnemyType type, float x, float y, float angle)
{
   if (isBomberman(type))
   {
      return;
   }

   Instance& instance = _instances[id];
   instance = Instance{};
   instance._type = type;
   instance._x = x;
   instance._y = y;
   instance._angle = angle;
   instance._target_angle = angle;
   instance._counted_x = x;
   instance._counted_y = y;

   // the walk cycles shouldn't all run in lockstep
   instance._time = static_cast<float>(id % 7) * 0.13f;
}

void EnemyRenderer::setPosition(int32_t id, float x, float y, float angle, float dx, float dy, int32_t flags)
{
   const auto it = _instances.find(id);

   if (it == _instances.end() || it->second._dying >= 0.0f)
   {
      return;
   }

   Instance& instance = it->second;
   instance._x = x;
   instance._y = y;
   instance._target_angle = angle;
   instance._vx = dx / SERVER_TICK;
   instance._vy = dy / SERVER_TICK;
   instance._since_update = 0.0f;
   instance._flags = flags;
}

void EnemyRenderer::hit(int32_t id)
{
   const auto it = _instances.find(id);

   if (it != _instances.end())
   {
      it->second._flash = FLASH_TIME;
   }
}

void EnemyRenderer::kill(int32_t id, bool removed)
{
   const auto it = _instances.find(id);

   if (it == _instances.end())
   {
      return;
   }

   if (removed)
   {
      _instances.erase(it);
      return;
   }

   it->second._dying = 0.0f;
   it->second._flash = FLASH_TIME;
   it->second._vx = 0.0f;
   it->second._vy = 0.0f;
}

void EnemyRenderer::clear()
{
   _instances.clear();
}

void EnemyRenderer::animate(float dt)
{
   for (auto it = _instances.begin(); it != _instances.end();)
   {
      Instance& instance = it->second;
      instance._time += dt;
      instance._flash = std::max(0.0f, instance._flash - dt);

      if (instance._dying >= 0.0f)
      {
         instance._dying += dt;

         if (instance._dying > DEATH_TIME)
         {
            it = _instances.erase(it);
            continue;
         }
      }
      else
      {
         // keep moving between the server's updates
         const float step = std::min(dt, std::max(0.0f, MAX_EXTRAPOLATION - instance._since_update));
         instance._x += instance._vx * step;
         instance._y += instance._vy * step;
         instance._since_update += dt;

         // the walk cycle follows the ground covered, so the feet don't slide whatever the speed
         const float covered = std::hypot(instance._x - instance._counted_x, instance._y - instance._counted_y);
         if (covered < MAX_STEP)
         {
            instance._walked += covered;
         }
         instance._standing = covered > 0.0f ? 0.0f : instance._standing + dt;
         instance._counted_x = instance._x;
         instance._counted_y = instance._y;

         const float delta = wrapAngle(instance._target_angle - instance._angle);
         instance._angle += delta * std::min(1.0f, TURN_RATE * dt);
      }

      ++it;
   }
}

void EnemyRenderer::render()
{
   if (_instances.empty())
   {
      return;
   }

   GltfModel::begin();

   for (const auto& [id, instance] : _instances)
   {
      Model& model = getModel(instance._type);

      if (!model._model->isValid())
      {
         continue;
      }

      const bool moving = (instance._flags & EnemyPositionPacket::FlagMoving) != 0;
      const bool shielded = (instance._flags & EnemyPositionPacket::FlagShielded) != 0;
      const bool invulnerable = (instance._flags & EnemyPositionPacket::FlagInvulnerable) != 0;

      const bool walking = moving && model._walk >= 0 && instance._standing < STAND_DELAY;
      int32_t animation = walking ? model._walk : model._idle;
      float time = instance._time;

      if (walking)
      {
         // the cycles shouldn't all run in lockstep either
         const float cycles = instance._walked / model._stride + static_cast<float>(id % 7) * 0.13f;
         time = getWalkTime(model, cycles);
      }

      GltfModel::Look look;
      look._flash = instance._flash / FLASH_TIME * 0.8f;

      Matrix fall;

      if (instance._dying >= 0.0f)
      {
         if (model._die >= 0)
         {
            // the death clip plays once and holds its last pose
            animation = model._die;
            time = std::min(instance._dying, model._model->getAnimationDuration(model._die) - 0.001f);
         }
         else
         {
            // no clip: tip over backwards
            const float t = std::min(instance._dying / FALL_TIME, 1.0f);
            fall = Matrix::rotateX(-0.5f * std::numbers::pi_v<float> * t * t);
         }

         look._alpha = std::clamp((DEATH_TIME - instance._dying) / (DEATH_TIME - FALL_TIME), 0.0f, 1.0f);
      }
      else if (shielded)
      {
         // the boss' force field
         const float pulse = 0.5f + 0.5f * std::sin(instance._time * 18.0f);
         look._tint = Vector(0.6f + 0.4f * pulse, 0.8f + 0.2f * pulse, 1.4f);
      }
      else if (invulnerable)
      {
         look._alpha = std::fmod(instance._time, 0.2f) < 0.1f ? 0.35f : 1.0f;
      }

      // model faces -y, the angle is 0 for +x
      const Matrix transform = fall * Matrix::scale(model._scale, model._scale, model._scale) *
                               Matrix::rotateZ(instance._angle + 0.5f * std::numbers::pi_v<float>) *
                               Matrix::position(instance._x, -instance._y, model._lift);

      model._model->draw(transform, animation, time, look);
   }

   GltfModel::end();
}

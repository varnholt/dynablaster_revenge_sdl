#include "enemy.h"

#include "enemyworld.h"

// shared
#include "random.h"

#include <algorithm>
#include <cmath>

namespace
{
constexpr float CENTRE_EPSILON = 0.001f;
constexpr float RETHINK_INTERVAL = 0.3f;
constexpr float HIT_INVULNERABILITY = 1.2f;

int32_t directionX(Constants::Direction direction)
{
   return direction == Constants::DirectionLeft ? -1 : (direction == Constants::DirectionRight ? 1 : 0);
}

int32_t directionY(Constants::Direction direction)
{
   return direction == Constants::DirectionUp ? -1 : (direction == Constants::DirectionDown ? 1 : 0);
}

Constants::Direction opposite(Constants::Direction direction)
{
   switch (direction)
   {
      case Constants::DirectionUp:
         return Constants::DirectionDown;
      case Constants::DirectionDown:
         return Constants::DirectionUp;
      case Constants::DirectionLeft:
         return Constants::DirectionRight;
      case Constants::DirectionRight:
         return Constants::DirectionLeft;
      default:
         return Constants::DirectionUnknown;
   }
}

float angleOf(Constants::Direction direction, float fallback)
{
   switch (direction)
   {
      case Constants::DirectionRight:
         return 0.0f;
      case Constants::DirectionUp:
         return 0.5f * std::numbers::pi_v<float>;
      case Constants::DirectionLeft:
         return std::numbers::pi_v<float>;
      case Constants::DirectionDown:
         return 1.5f * std::numbers::pi_v<float>;
      default:
         return fallback;
   }
}
}  // namespace

Enemy::Enemy(int16_t id, EnemyType type, EnemyWorld& world) : _world(world), _id(id), _type(type)
{
}

Enemy::~Enemy() = default;

void Enemy::initialize()
{
}

void Enemy::update(float dt)
{
   if (_dead)
   {
      return;
   }

   if (_invulnerable_time > 0.0f)
   {
      _invulnerable_time = std::max(0.0f, _invulnerable_time - dt);

      if (_invulnerable_time == 0.0f)
      {
         _changed = true;
      }
   }

   // timers may start new timers, so collect the due ones first
   std::vector<int32_t> due;
   for (PendingTimer& timer : _timers)
   {
      timer._remaining -= dt * 1000.0f;

      if (timer._remaining <= 0.0f)
      {
         due.push_back(timer._id);
      }
   }

   std::erase_if(_timers, [](const PendingTimer& timer) { return timer._remaining <= 0.0f; });

   for (const int32_t id : due)
   {
      if (!_dead)
      {
         timeout(id);
      }
   }

   if (_dead)
   {
      return;
   }

   tick(dt);
   move(dt);
}

bool Enemy::hit(int8_t player_id)
{
   if (_dead || _shielded || _invulnerable_time > 0.0f)
   {
      return false;
   }

   _hit_points--;

   if (_hit_points > 0)
   {
      _invulnerable_time = HIT_INVULNERABILITY;
      _was_hit = true;
      _changed = true;
      hurt();
      return false;
   }

   _dead = true;
   _killer_id = player_id;
   died();
   return true;
}

void Enemy::remove()
{
   _dead = true;
   _removed = true;
}

void Enemy::move(float dt)
{
   _moved = false;

   if (_direction == Constants::DirectionUnknown)
   {
      _rethink_time -= dt;

      if (_rethink_time <= 0.0f && isAtCentre())
      {
         chooseDirection();
      }

      if (_direction == Constants::DirectionUnknown)
      {
         return;
      }
   }

   float distance = _speed * dt;

   // a step may cross several centres at high speed; bounded so a boxed in enemy can't spin forever
   for (int32_t step = 0; step < 8 && distance > 0.0f; step++)
   {
      if (isAtCentre())
      {
         _x = std::floor(_x) + 0.5f;
         _y = std::floor(_y) + 0.5f;

         chooseDirection();

         if (_direction == Constants::DirectionUnknown)
         {
            break;
         }
      }

      const bool horizontal = directionX(_direction) != 0;
      const float sign = static_cast<float>(horizontal ? directionX(_direction) : directionY(_direction));
      float& along = horizontal ? _x : _y;

      // the next tile centre ahead
      const float base = along - 0.5f;
      const float next = (sign > 0.0f ? std::floor(base + CENTRE_EPSILON) + 1.0f : std::ceil(base - CENTRE_EPSILON) - 1.0f) + 0.5f;

      const auto target = static_cast<int32_t>(std::floor(next));
      const int32_t target_x = horizontal ? target : getTileX();
      const int32_t target_y = horizontal ? getTileY() : target;
      const bool inside_target = (horizontal ? getTileX() : getTileY()) == target;

      // something appeared in the way (a bomb) before we got there: turn around
      if (!inside_target && !_world.isWalkable(target_x, target_y, _wall_pass))
      {
         _direction = opposite(_direction);
         _changed = true;
         continue;
      }

      const float remaining = std::abs(next - along);
      const float travelled = std::min(distance, remaining);

      along += sign * travelled;
      distance -= travelled;
      _moved = true;

      if (travelled >= remaining)
      {
         along = next;
      }
   }

   if (_moved)
   {
      _angle = angleOf(_direction, _angle);
      _changed = true;
   }
}

void Enemy::chooseDirection()
{
   int32_t blocked = 0;

   for (const Constants::Direction direction :
        {Constants::DirectionUp, Constants::DirectionDown, Constants::DirectionLeft, Constants::DirectionRight})
   {
      if (!canMove(direction))
      {
         blocked |= 1 << direction;
      }
   }

   Constants::Direction direction = think(blocked);

   if (direction != Constants::DirectionUnknown && (blocked & (1 << direction)))
   {
      direction = Constants::DirectionUnknown;
   }

   if (direction != _direction)
   {
      _changed = true;
   }

   _direction = direction;

   if (_direction == Constants::DirectionUnknown)
   {
      _rethink_time = RETHINK_INTERVAL;
   }
}

bool Enemy::isAtCentre() const
{
   const float fx = _x - std::floor(_x);
   const float fy = _y - std::floor(_y);
   return std::abs(fx - 0.5f) < CENTRE_EPSILON && std::abs(fy - 0.5f) < CENTRE_EPSILON;
}

Constants::Direction Enemy::think(int32_t blocked)
{
   // keep walking straight while possible, otherwise pick any open way
   if (_direction != Constants::DirectionUnknown && !(blocked & (1 << _direction)))
   {
      return _direction;
   }

   const std::vector<Constants::Direction> free = getFreeDirections();

   if (free.empty())
   {
      return Constants::DirectionUnknown;
   }

   return free[static_cast<size_t>(Random::bounded(static_cast<int32_t>(free.size())))];
}

void Enemy::tick(float /*dt*/)
{
}

void Enemy::timeout(int32_t /*id*/)
{
}

void Enemy::hurt()
{
}

void Enemy::died()
{
}

std::vector<Constants::Direction> Enemy::getFreeDirections() const
{
   std::vector<Constants::Direction> free;

   for (const Constants::Direction direction :
        {Constants::DirectionUp, Constants::DirectionDown, Constants::DirectionLeft, Constants::DirectionRight})
   {
      if (canMove(direction))
      {
         free.push_back(direction);
      }
   }

   return free;
}

bool Enemy::canMove(Constants::Direction direction) const
{
   if (direction == Constants::DirectionUnknown)
   {
      return false;
   }

   return _world.isWalkable(getTileX() + directionX(direction), getTileY() + directionY(direction), _wall_pass);
}

void Enemy::startTimer(int32_t ms, int32_t id)
{
   _timers.push_back({static_cast<float>(ms), id});
}

int16_t Enemy::getId() const
{
   return _id;
}

EnemyType Enemy::getType() const
{
   return _type;
}

int16_t Enemy::getParentId() const
{
   return _parent_id;
}

void Enemy::setParentId(int16_t id)
{
   _parent_id = id;
}

float Enemy::getX() const
{
   return _x;
}

float Enemy::getY() const
{
   return _y;
}

int32_t Enemy::getTileX() const
{
   return static_cast<int32_t>(std::floor(_x));
}

int32_t Enemy::getTileY() const
{
   return static_cast<int32_t>(std::floor(_y));
}

void Enemy::setPosition(float x, float y)
{
   _x = x;
   _y = y;
   _changed = true;
}

float Enemy::getAngle() const
{
   return _angle;
}

Constants::Direction Enemy::getDirection() const
{
   return _direction;
}

void Enemy::setDirection(Constants::Direction direction)
{
   _direction = direction;
   _changed = true;
}

float Enemy::getSpeed() const
{
   return _speed;
}

void Enemy::setSpeed(float speed)
{
   _speed = speed;
}

bool Enemy::hasWallPass() const
{
   return _wall_pass;
}

void Enemy::setWallPass(bool wall_pass)
{
   _wall_pass = wall_pass;
}

int32_t Enemy::getHitPoints() const
{
   return _hit_points;
}

void Enemy::setHitPoints(int32_t hit_points)
{
   _hit_points = hit_points;
}

int32_t Enemy::getPoints() const
{
   return _points;
}

void Enemy::setPoints(int32_t points)
{
   _points = points;
}

bool Enemy::isShielded() const
{
   return _shielded;
}

void Enemy::setShielded(bool shielded)
{
   if (_shielded != shielded)
   {
      _changed = true;
   }

   _shielded = shielded;
}

bool Enemy::isInvulnerable() const
{
   return _invulnerable_time > 0.0f;
}

void Enemy::protect(float seconds)
{
   _invulnerable_time = std::max(_invulnerable_time, seconds);
   _changed = true;
}

bool Enemy::isDead() const
{
   return _dead;
}

bool Enemy::isRemoved() const
{
   return _removed;
}

int8_t Enemy::getKillerId() const
{
   return _killer_id;
}

bool Enemy::isMoving() const
{
   return _moved;
}

bool Enemy::takeChanged()
{
   const bool changed = _changed;
   _changed = false;
   return changed;
}

bool Enemy::takeHit()
{
   const bool was_hit = _was_hit;
   _was_hit = false;
   return was_hit;
}

EnemyWorld& Enemy::getWorld() const
{
   return _world;
}

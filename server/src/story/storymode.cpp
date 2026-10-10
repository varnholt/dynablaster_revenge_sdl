#include "storymode.h"

// server
#include "enemies/enemy.h"
#include "enemies/enemyfactory.h"
#include "game.h"

// shared
#include "blockmapitem.h"
#include "bombmapitem.h"
#include "detonationpacket.h"
#include "enemycreatedpacket.h"
#include "enemyhitpacket.h"
#include "enemykilledpacket.h"
#include "enemypositionpacket.h"
#include "extramapitem.h"
#include "gameeventpacket.h"
#include "logging.h"
#include "map.h"
#include "player.h"
#include "random.h"
#include "stonemapitem.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <queue>

namespace
{
// share of the free tiles covered by soft blocks
constexpr float SOFT_BLOCK_DENSITY = 0.36f;

// enemies keep this far from the player's start (manhattan)
constexpr int32_t SPAWN_DISTANCE = 5;

// too many enemies make the field unplayable when the exit gets bombed over and over
constexpr int32_t MAX_ENEMIES = 48;

constexpr int32_t STAGE_CLEARED_DELAY = 3000;
constexpr int32_t LIFE_LOST_DELAY = 3000;
constexpr float BOSS_CLEARED_DELAY = 2.0f;

constexpr std::array<Constants::Direction, 4> DIRECTIONS{
   Constants::DirectionUp,
   Constants::DirectionDown,
   Constants::DirectionLeft,
   Constants::DirectionRight
};

int32_t stepX(Constants::Direction direction)
{
   return direction == Constants::DirectionLeft ? -1 : (direction == Constants::DirectionRight ? 1 : 0);
}

int32_t stepY(Constants::Direction direction)
{
   return direction == Constants::DirectionUp ? -1 : (direction == Constants::DirectionDown ? 1 : 0);
}

int32_t tileOf(float value)
{
   return static_cast<int32_t>(std::floor(value));
}
}  // namespace

StoryMode::StoryMode(Game& game, int32_t first_stage)
    : _game(game), _stage_index(std::clamp(first_stage, 0, static_cast<int32_t>(getStoryStages().size()) - 1))
{
}

StoryMode::~StoryMode() = default;

const StoryStage& StoryMode::getStage() const
{
   return getStoryStages()[static_cast<size_t>(_stage_index)];
}

int32_t StoryMode::getStageIndex() const
{
   return _stage_index;
}

std::unique_ptr<Map> StoryMode::createMap()
{
   const StoryStage& stage = getStage();
   const int32_t width = stage.getWidth();
   const int32_t height = stage.getHeight();

   auto map = std::make_unique<Map>(width, height);
   map->initialize();
   map->setStartPositions({Point(0, 0)});

   _flame_time.assign(static_cast<size_t>(width * height), 0.0f);
   _flame_owner.assign(static_cast<size_t>(width * height), 0);
   _exit.reset();
   _cleared = false;
   _time_up = false;
   _boss_clear_delay = -1.0f;

   // the player starts in the top left corner with room to escape its first bomb
   const auto is_start_area = [](int32_t x, int32_t y) { return x + y <= 1 || (x == 2 && y == 0) || (x == 0 && y == 2); };

   std::vector<Point> free_tiles;
   for (int32_t y = 0; y < height; y++)
   {
      for (int32_t x = 0; x < width; x++)
      {
         if (!map->getItem(x, y) && !is_start_area(x, y))
         {
            free_tiles.emplace_back(x, y);
         }
      }
   }

   // boss arenas are open
   if (stage.isBossStage())
   {
      return map;
   }

   const auto soft_blocks = static_cast<int32_t>(static_cast<float>(free_tiles.size()) * SOFT_BLOCK_DENSITY);

   std::vector<std::reference_wrapper<StoneMapItem>> stones;
   for (int32_t i = 0; i < soft_blocks && !free_tiles.empty(); i++)
   {
      const auto index = static_cast<size_t>(Random::bounded(static_cast<int32_t>(free_tiles.size())));
      const Point tile = free_tiles[index];
      free_tiles.erase(free_tiles.begin() + static_cast<std::ptrdiff_t>(index));

      auto stone = std::make_shared<StoneMapItem>(-1, tile.x(), tile.y());
      stones.emplace_back(*stone);
      map->setItem(tile.x(), tile.y(), std::move(stone));
   }

   // one soft block hides the exit, another one the stage's item
   if (stones.size() >= 2)
   {
      const auto exit_index = static_cast<size_t>(Random::bounded(static_cast<int32_t>(stones.size())));
      StoneMapItem& exit_stone = stones[exit_index];
      exit_stone.setExtraMapItem(std::make_unique<ExtraMapItem>(-1, Constants::ExtraExit, exit_stone.getX(), exit_stone.getY()));
      _exit = Point(exit_stone.getX(), exit_stone.getY());
      stones.erase(stones.begin() + static_cast<std::ptrdiff_t>(exit_index));

      if (stage._item != 0)
      {
         StoneMapItem& item_stone = stones[static_cast<size_t>(Random::bounded(static_cast<int32_t>(stones.size())))];
         item_stone.setExtraMapItem(std::make_unique<ExtraMapItem>(
            -1, static_cast<Constants::ExtraType>(stage._item), item_stone.getX(), item_stone.getY()
         ));
      }
   }

   return map;
}

void StoryMode::applyPlayerState(Player& player) const
{
   player.setBombCount(static_cast<int8_t>(_abilities._bombs));
   player.setFlameCount(static_cast<int8_t>(_abilities._flames));
   player.setSpeed(_abilities._speed);
   player.setRemoteControl(_abilities._remote);
   player.setWallPass(_abilities._wall_pass);
   player.setBombPass(_abilities._bomb_pass);
}

void StoryMode::prepareStage()
{
   removeAllEnemies();
   spawnStageEnemies();
   sendState(StoryStatePacket::StateStageIntro);
}

void StoryMode::startStage()
{
   sendState(StoryStatePacket::StatePlaying);
}

void StoryMode::removeAllEnemies()
{
   for (const auto& enemy : _enemies)
   {
      _game.addOutgoingPacket(std::make_unique<EnemyKilledPacket>(enemy->getId(), -1, EnemyKilledPacket::ReasonRemoved, 0));
   }

   _enemies.clear();
   _spawned.clear();
}

void StoryMode::spawnStageEnemies()
{
   const StoryStage& stage = getStage();
   const int32_t width = getWidth();
   const int32_t height = getHeight();

   std::vector<Point> candidates;
   for (int32_t y = 0; y < height; y++)
   {
      for (int32_t x = 0; x < width; x++)
      {
         if (!getMap().getItem(x, y) && x + y >= SPAWN_DISTANCE)
         {
            candidates.emplace_back(x, y);
         }
      }
   }

   for (const EnemyType type : stage._enemies)
   {
      if (candidates.empty())
      {
         break;
      }

      // bosses take the middle of the arena
      size_t index = static_cast<size_t>(Random::bounded(static_cast<int32_t>(candidates.size())));
      if (isBoss(type))
      {
         const Point centre(width / 2, height / 2);
         index = static_cast<size_t>(std::distance(
            candidates.begin(),
            std::ranges::min_element(
               candidates, {}, [&centre](const Point& p) { return Map::getManhattanLength(p.x(), p.y(), centre.x(), centre.y()); }
            )
         ));
      }

      const Point tile = candidates[index];
      candidates.erase(candidates.begin() + static_cast<std::ptrdiff_t>(index));
      spawnEnemy(type, static_cast<float>(tile.x()) + 0.5f, static_cast<float>(tile.y()) + 0.5f, -1);
   }
}

int16_t StoryMode::spawnEnemy(EnemyType type, float x, float y, int16_t parent_id)
{
   if (aliveEnemyCount() + static_cast<int32_t>(_spawned.size()) >= MAX_ENEMIES)
   {
      return -1;
   }

   const int16_t id = _next_enemy_id++;
   auto enemy = EnemyFactory::create(id, type, *this);
   enemy->setParentId(parent_id);
   enemy->setPosition(static_cast<float>(tileOf(x)) + 0.5f, static_cast<float>(tileOf(y)) + 0.5f);

   _game.addOutgoingPacket(std::make_unique<EnemyCreatedPacket>(
      id, static_cast<int8_t>(type), enemy->getX(), enemy->getY(), enemy->getAngle()
   ));

   // enemies spawned while the list is being updated join afterwards
   _spawned.push_back(std::move(enemy));
   return id;
}

void StoryMode::spawnAround(int32_t x, int32_t y, int32_t count)
{
   std::vector<EnemyType> types;
   for (const EnemyType type : getStage()._enemies)
   {
      if (!isBoss(type) && type != EnemyType::Bomberman)
      {
         types.push_back(type);
      }
   }

   if (types.empty())
   {
      types.push_back(EnemyType::Pontan);
   }

   // all of one kind, like in the original
   const EnemyType type = types[static_cast<size_t>(Random::bounded(static_cast<int32_t>(types.size())))];

   for (int32_t i = 0; i < count; i++)
   {
      // they appear in the flames that set them free
      if (spawnEnemy(type, static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f, -1) >= 0)
      {
         _spawned.back()->protect(STORY_SPAWN_PROTECTION);
      }
   }
}

void StoryMode::update(float dt)
{
   updateEnemies(dt);
   updateFlames(dt);
   updateTouches();
   updateStageCleared();
}

void StoryMode::updateEnemies(float dt)
{
   for (auto& enemy : _spawned)
   {
      _enemies.push_back(std::move(enemy));
   }
   _spawned.clear();

   for (size_t i = 0; i < _enemies.size(); i++)
   {
      Enemy& enemy = *_enemies[i];
      enemy.update(dt);

      if (enemy.isDead())
      {
         continue;
      }

      if (enemy.takeHit())
      {
         _game.addOutgoingPacket(std::make_unique<EnemyHitPacket>(enemy.getId(), static_cast<int8_t>(enemy.getHitPoints())));
      }

      if (enemy.takeChanged())
      {
         int8_t flags = 0;
         flags |= enemy.isMoving() ? EnemyPositionPacket::FlagMoving : 0;
         flags |= enemy.isShielded() ? EnemyPositionPacket::FlagShielded : 0;
         flags |= enemy.isInvulnerable() ? EnemyPositionPacket::FlagInvulnerable : 0;

         const float speed = enemy.isMoving() ? enemy.getSpeed() * dt : 0.0f;
         const float dx = static_cast<float>(stepX(enemy.getDirection())) * speed;
         const float dy = static_cast<float>(stepY(enemy.getDirection())) * speed;

         _game.addOutgoingPacket(
            std::make_unique<EnemyPositionPacket>(enemy.getId(), enemy.getX(), enemy.getY(), enemy.getAngle(), dx, dy, flags)
         );
      }
   }

   // children of a fallen boss go with it (warp man's blobs, bubbles' clouds)
   for (const auto& enemy : _enemies)
   {
      if (enemy->isDead() || enemy->getParentId() < 0)
      {
         continue;
      }

      const auto parent = std::ranges::find_if(_enemies, [&enemy](const auto& other) { return other->getId() == enemy->getParentId(); });

      if (parent == _enemies.end() || (*parent)->isDead())
      {
         enemy->remove();
      }
   }

   for (const auto& enemy : _enemies)
   {
      if (enemy->isDead())
      {
         enemyDied(*enemy);
      }
   }

   std::erase_if(_enemies, [](const auto& enemy) { return enemy->isDead(); });

   // compared with what the clients were told, flames kill enemies outside of this update
   if (enemiesLeft() != _sent_enemies_left)
   {
      sendState(StoryStatePacket::StatePlaying);
   }
}

void StoryMode::enemyDied(Enemy& enemy)
{
   if (enemy.isRemoved())
   {
      _game.addOutgoingPacket(std::make_unique<EnemyKilledPacket>(enemy.getId(), -1, EnemyKilledPacket::ReasonRemoved, 0));
      return;
   }

   _score += enemy.getPoints();
   _game.addOutgoingPacket(
      std::make_unique<EnemyKilledPacket>(enemy.getId(), enemy.getKillerId(), EnemyKilledPacket::ReasonKilled, enemy.getPoints())
   );
}

void StoryMode::flameReached(int32_t x, int32_t y, int8_t owner_id)
{
   const auto index = static_cast<size_t>(y * getWidth() + x);

   if (index >= _flame_time.size())
   {
      return;
   }

   _flame_time[index] = static_cast<float>(STORY_FLAME_LINGER_TIME);
   _flame_owner[index] = owner_id;

   if (owner_id == ENEMY_OWNER_ID)
   {
      return;
   }

   for (const auto& enemy : _enemies)
   {
      if (enemy->getTileX() == x && enemy->getTileY() == y)
      {
         enemy->hit(owner_id);
      }
   }
}

void StoryMode::updateFlames(float dt)
{
   const int32_t width = getWidth();

   for (size_t index = 0; index < _flame_time.size(); index++)
   {
      if (_flame_time[index] <= 0.0f)
      {
         continue;
      }

      _flame_time[index] -= dt * 1000.0f;

      const auto x = static_cast<int32_t>(index) % width;
      const auto y = static_cast<int32_t>(index) / width;

      // enemies walking into a burning tile die as well
      if (_flame_owner[index] != ENEMY_OWNER_ID)
      {
         for (const auto& enemy : _enemies)
         {
            if (!enemy->isDead() && enemy->getTileX() == x && enemy->getTileY() == y)
            {
               enemy->hit(_flame_owner[index]);
            }
         }
      }

      for (Player& player : _game.getPlayers())
      {
         if (!player.isKilled() && tileOf(player.getX()) == x && tileOf(player.getY()) == y)
         {
            killPlayer(player);
         }
      }
   }
}

void StoryMode::updateTouches()
{
   for (Player& player : _game.getPlayers())
   {
      if (player.isKilled())
      {
         continue;
      }

      for (const auto& enemy : _enemies)
      {
         const float dx = enemy->getX() - player.getX();
         const float dy = enemy->getY() - player.getY();

         if (!enemy->isDead() && dx * dx + dy * dy < STORY_TOUCH_DISTANCE * STORY_TOUCH_DISTANCE)
         {
            killPlayer(player);
            break;
         }
      }
   }
}

void StoryMode::killPlayer(Player& player)
{
   if (!player.isInvincible() && !player.isKilled())
   {
      _game.killPlayerByEnemy(player);
   }
}

void StoryMode::updateStageCleared()
{
   if (_cleared || aliveEnemyCount() > 0 || !_spawned.empty())
   {
      return;
   }

   // boss arenas have no exit, they are won once everything is gone
   if (getStage().isBossStage())
   {
      if (_boss_clear_delay < 0.0f)
      {
         _boss_clear_delay = BOSS_CLEARED_DELAY;
      }

      _boss_clear_delay -= 1.0f / static_cast<float>(SERVER_HEARTBEAT_IN_HZ);

      if (_boss_clear_delay <= 0.0f)
      {
         stageCleared();
      }

      return;
   }

   if (!_exit)
   {
      return;
   }

   const auto& item = getMap().getItem(_exit->x(), _exit->y());
   const bool exit_open = item && item->getType() == MapItem::Extra &&
                          static_cast<const ExtraMapItem&>(*item).getExtraType() == Constants::ExtraExit;

   if (exit_open && isPlayerOn(_exit->x(), _exit->y()))
   {
      stageCleared();
   }
}

void StoryMode::stageCleared()
{
   _cleared = true;

   // the abilities found so far are kept for the next stage
   for (Player& player : _game.getPlayers())
   {
      _abilities._bombs = player.getBombCount();
      _abilities._flames = player.getFlameCount();
      _abilities._speed = player.getSpeed();
      _abilities._remote = player.hasRemoteControl();
      _abilities._wall_pass = player.hasWallPass();
      _abilities._bomb_pass = player.hasBombPass();
   }

   sendState(StoryStatePacket::StateStageCleared);

   if (_stage_index + 1 >= static_cast<int32_t>(getStoryStages().size()))
   {
      _completed = true;
      sendState(StoryStatePacket::StateCompleted);
   }
   else
   {
      _stage_index++;
   }

   _game.finishGame();
}

void StoryMode::exitBombed(int32_t x, int32_t y)
{
   spawnAround(x, y, STORY_BOMBED_ITEM_ENEMIES);
}

void StoryMode::itemBombed(int32_t x, int32_t y)
{
   spawnAround(x, y, STORY_BOMBED_ITEM_ENEMIES);
}

void StoryMode::extraCollected(Player& player, int32_t extra_type)
{
   switch (extra_type)
   {
      case Constants::ExtraWallPass:
         player.setWallPass(true);
         break;
      case Constants::ExtraBombPass:
         player.setBombPass(true);
         break;
      case Constants::ExtraRemote:
         player.setRemoteControl(true);
         break;
      case Constants::ExtraVest:
         player.setVestTime(STORY_VEST_DURATION);
         break;
      case Constants::ExtraOneUp:
         _lives++;
         sendState(StoryStatePacket::StatePlaying);
         break;
      default:
         break;
   }
}

void StoryMode::playersDied()
{
   if (_cleared)
   {
      return;
   }

   _lives--;

   // bomb and fire ups survive a death, everything else is gone
   _abilities._speed = SERVER_DEFAULT_SPEED;
   _abilities._remote = false;
   _abilities._wall_pass = false;
   _abilities._bomb_pass = false;

   if (_lives <= 0)
   {
      _game_over = true;
      sendState(StoryStatePacket::StateGameOver);
   }
   else
   {
      sendState(StoryStatePacket::StateLifeLost);
   }

   _game.finishGame();
}

void StoryMode::timeUp()
{
   if (_time_up || _cleared)
   {
      return;
   }

   _time_up = true;

   for (const auto& enemy : _enemies)
   {
      enemy->remove();
   }

   for (int32_t i = 0; i < STORY_TIMEOUT_PONTANS; i++)
   {
      if (const auto tile = getRandomFreeTile(SPAWN_DISTANCE))
      {
         spawnEnemy(EnemyType::Pontan, static_cast<float>(tile->x()) + 0.5f, static_cast<float>(tile->y()) + 0.5f, -1);
      }
   }
}

bool StoryMode::isFinished() const
{
   return _game_over || _completed;
}

int32_t StoryMode::getNextStageDelay() const
{
   return _cleared ? STAGE_CLEARED_DELAY : LIFE_LOST_DELAY;
}

void StoryMode::sendState(StoryStatePacket::State state)
{
   _game.addOutgoingPacket(std::make_unique<StoryStatePacket>(
      static_cast<int8_t>(_stage_index),
      static_cast<int8_t>(std::clamp(_lives, 0, 99)),
      _score,
      static_cast<int8_t>(state),
      static_cast<int8_t>(std::min(enemiesLeft(), 127))
   ));

   _sent_enemies_left = enemiesLeft();
}

int32_t StoryMode::enemiesLeft() const
{
   return aliveEnemyCount() + static_cast<int32_t>(_spawned.size());
}

int32_t StoryMode::aliveEnemyCount() const
{
   return static_cast<int32_t>(std::ranges::count_if(_enemies, [](const auto& enemy) { return !enemy->isDead(); }));
}

Map& StoryMode::getMap() const
{
   return _game.getMap();
}

bool StoryMode::isPlayerOn(int32_t x, int32_t y) const
{
   return std::ranges::any_of(
      _game.getPlayers(),
      [x, y](const Player& player) { return !player.isKilled() && tileOf(player.getX()) == x && tileOf(player.getY()) == y; }
   );
}

int32_t StoryMode::getWidth() const
{
   return getStage().getWidth();
}

int32_t StoryMode::getHeight() const
{
   return getStage().getHeight();
}

bool StoryMode::isWalkable(int32_t x, int32_t y, bool wall_pass) const
{
   if (x < 0 || y < 0 || x >= getWidth() || y >= getHeight())
   {
      return false;
   }

   const auto& item = getMap().getItem(x, y);

   if (!item)
   {
      return true;
   }

   switch (item->getType())
   {
      case MapItem::Extra:
         return true;
      case MapItem::Stone:
         return wall_pass;
      default:
         return false;
   }
}

bool StoryMode::isDangerous(int32_t x, int32_t y) const
{
   const Map& map = getMap();

   for (int32_t by = 0; by < getHeight(); by++)
   {
      for (int32_t bx = 0; bx < getWidth(); bx++)
      {
         const auto& item = map.getItem(bx, by);

         if (!item || item->getType() != MapItem::Bomb || (bx != x && by != y))
         {
            continue;
         }

         const int32_t distance = std::abs(bx - x) + std::abs(by - y);
         const int32_t flames = static_cast<const BombMapItem&>(*item).getFlames();

         if (distance > flames)
         {
            continue;
         }

         // the flame stops at the first obstacle in between
         bool covered = true;
         const int32_t sx = (x > bx) - (x < bx);
         const int32_t sy = (y > by) - (y < by);
         for (int32_t step = 1; step < distance; step++)
         {
            const auto& between = map.getItem(bx + sx * step, by + sy * step);
            if (between && between->isBlocking())
            {
               covered = false;
               break;
            }
         }

         if (covered)
         {
            return true;
         }
      }
   }

   const auto index = static_cast<size_t>(y * getWidth() + x);
   return index < _flame_time.size() && _flame_time[index] > 0.0f;
}

std::optional<Point> StoryMode::getNearestPlayer(float x, float y) const
{
   std::optional<Point> nearest;
   float best = 0.0f;

   for (const Player& player : _game.getPlayers())
   {
      if (player.isKilled())
      {
         continue;
      }

      const float dx = player.getX() - x;
      const float dy = player.getY() - y;
      const float distance = dx * dx + dy * dy;

      if (!nearest || distance < best)
      {
         best = distance;
         nearest = Point(tileOf(player.getX()), tileOf(player.getY()));
      }
   }

   return nearest;
}

Constants::Direction StoryMode::getDirectionToPlayer(const Enemy& enemy, int32_t max_steps) const
{
   const int32_t width = getWidth();
   const int32_t height = getHeight();
   const int32_t start = enemy.getTileY() * width + enemy.getTileX();

   std::vector<int32_t> distance(static_cast<size_t>(width * height), -1);
   std::vector<Constants::Direction> first_step(static_cast<size_t>(width * height), Constants::DirectionUnknown);
   std::queue<int32_t> open;

   distance[static_cast<size_t>(start)] = 0;
   open.push(start);

   // breadth first: the first player tile reached is the closest by walking
   while (!open.empty())
   {
      const int32_t current = open.front();
      open.pop();

      const int32_t cx = current % width;
      const int32_t cy = current / width;

      if (current != start && isPlayerOn(cx, cy))
      {
         return first_step[static_cast<size_t>(current)];
      }

      if (distance[static_cast<size_t>(current)] >= max_steps)
      {
         continue;
      }

      for (const Constants::Direction direction : DIRECTIONS)
      {
         const int32_t nx = cx + stepX(direction);
         const int32_t ny = cy + stepY(direction);

         if (!isWalkable(nx, ny, enemy.hasWallPass()))
         {
            continue;
         }

         const int32_t next = ny * width + nx;

         if (distance[static_cast<size_t>(next)] >= 0)
         {
            continue;
         }

         distance[static_cast<size_t>(next)] = distance[static_cast<size_t>(current)] + 1;
         first_step[static_cast<size_t>(next)] = current == start ? direction : first_step[static_cast<size_t>(current)];
         open.push(next);
      }
   }

   return Constants::DirectionUnknown;
}

std::optional<Point> StoryMode::getRandomFreeTile(int32_t min_player_distance) const
{
   std::vector<Point> candidates;

   for (int32_t y = 0; y < getHeight(); y++)
   {
      for (int32_t x = 0; x < getWidth(); x++)
      {
         if (getMap().getItem(x, y))
         {
            continue;
         }

         const auto player = getNearestPlayer(static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f);

         if (!player || Map::getManhattanLength(x, y, player->x(), player->y()) >= min_player_distance)
         {
            candidates.emplace_back(x, y);
         }
      }
   }

   if (candidates.empty())
   {
      return std::nullopt;
   }

   return candidates[static_cast<size_t>(Random::bounded(static_cast<int32_t>(candidates.size())))];
}

int32_t StoryMode::countEnemies(EnemyType type) const
{
   const auto alive_of_type = [type](const auto& enemy) { return !enemy->isDead() && enemy->getType() == type; };
   return static_cast<int32_t>(std::ranges::count_if(_enemies, alive_of_type) + std::ranges::count_if(_spawned, alive_of_type));
}

void StoryMode::spitFire(int32_t x, int32_t y, int32_t range)
{
   std::array<int32_t, 4> reach{};

   flameReached(x, y, ENEMY_OWNER_ID);

   // the fire stops at walls and soft blocks without burning them
   for (size_t d = 0; d < DIRECTIONS.size(); d++)
   {
      for (int32_t step = 1; step <= range; step++)
      {
         const int32_t fx = x + stepX(DIRECTIONS[d]) * step;
         const int32_t fy = y + stepY(DIRECTIONS[d]) * step;

         if (fx < 0 || fy < 0 || fx >= getWidth() || fy >= getHeight())
         {
            break;
         }

         const auto& item = getMap().getItem(fx, fy);
         if (item && item->isBlocking())
         {
            break;
         }

         flameReached(fx, fy, ENEMY_OWNER_ID);
         reach[d] = step;
      }
   }

   _game.addOutgoingPacket(std::make_unique<GameEventPacket>(GameEventPacket::BombExploded, 0.4f));
   _game.addOutgoingPacket(std::make_unique<DetonationPacket>(
      x,
      y,
      static_cast<int8_t>(reach[0]),
      static_cast<int8_t>(reach[1]),
      static_cast<int8_t>(reach[2]),
      static_cast<int8_t>(reach[3]),
      static_cast<float>(range)
   ));
}

void StoryMode::dropBomb(int32_t x, int32_t y, int32_t flames)
{
   _game.placeBomb(ENEMY_OWNER_ID, flames, x, y);
}

#include "scriptedenemy.h"

#include "enemyworld.h"

// shared
#include "logging.h"
#include "random.h"

extern "C"
{
#include "lauxlib.h"
#include "lua.h"
#include "lualib.h"
}

#include <array>
#include <cmath>
#include <fstream>
#include <optional>
#include <sstream>

namespace
{
constexpr char SCRIPT_DIRECTORY[] = "data/scripts/enemies/";

ScriptedEnemy& self(lua_State* state)
{
   return *static_cast<ScriptedEnemy*>(lua_touserdata(state, lua_upvalueindex(1)));
}

Constants::Direction toDirection(lua_Integer value)
{
   return (value >= Constants::DirectionUp && value <= Constants::DirectionRight) ? static_cast<Constants::Direction>(value)
                                                                                   : Constants::DirectionUnknown;
}

int32_t directionX(Constants::Direction direction)
{
   return direction == Constants::DirectionLeft ? -1 : (direction == Constants::DirectionRight ? 1 : 0);
}

int32_t directionY(Constants::Direction direction)
{
   return direction == Constants::DirectionUp ? -1 : (direction == Constants::DirectionDown ? 1 : 0);
}

std::optional<EnemyType> typeArgument(lua_State* state, int index)
{
   const char* name = luaL_checkstring(state, index);
   return getEnemyType(name);
}

// --- functions the scripts can call ---------------------------------------------------------
namespace api
{

int direction(lua_State* state)
{
   lua_pushinteger(state, self(state).getDirection());
   return 1;
}

int setDirection(lua_State* state)
{
   self(state).setDirection(toDirection(luaL_checkinteger(state, 1)));
   return 0;
}

int speed(lua_State* state)
{
   lua_pushnumber(state, self(state).getSpeed());
   return 1;
}

int setSpeed(lua_State* state)
{
   self(state).setSpeed(static_cast<float>(luaL_checknumber(state, 1)));
   return 0;
}

int setWallPass(lua_State* state)
{
   self(state).setWallPass(lua_toboolean(state, 1) != 0);
   return 0;
}

int position(lua_State* state)
{
   lua_pushnumber(state, self(state).getX());
   lua_pushnumber(state, self(state).getY());
   return 2;
}

int tile(lua_State* state)
{
   lua_pushinteger(state, self(state).getTileX());
   lua_pushinteger(state, self(state).getTileY());
   return 2;
}

int canMove(lua_State* state)
{
   lua_pushboolean(state, self(state).canMove(toDirection(luaL_checkinteger(state, 1))));
   return 1;
}

int freeDirections(lua_State* state)
{
   const auto free = self(state).getFreeDirections();
   lua_createtable(state, static_cast<int>(free.size()), 0);

   for (size_t i = 0; i < free.size(); i++)
   {
      lua_pushinteger(state, free[i]);
      lua_rawseti(state, -2, static_cast<lua_Integer>(i + 1));
   }

   return 1;
}

int randomNumber(lua_State* state)
{
   const auto bound = static_cast<int32_t>(luaL_checkinteger(state, 1));
   lua_pushinteger(state, bound > 0 ? Random::bounded(bound) : 0);
   return 1;
}

int nearestPlayer(lua_State* state)
{
   const ScriptedEnemy& enemy = self(state);
   const auto player = enemy.getWorld().getNearestPlayer(enemy.getX(), enemy.getY());

   if (!player)
   {
      return 0;
   }

   lua_pushinteger(state, player->x());
   lua_pushinteger(state, player->y());
   lua_pushinteger(state, std::abs(player->x() - enemy.getTileX()) + std::abs(player->y() - enemy.getTileY()));
   return 3;
}

int directionToPlayer(lua_State* state)
{
   const auto max_steps = static_cast<int32_t>(luaL_optinteger(state, 1, 64));
   lua_pushinteger(state, self(state).getWorld().getDirectionToPlayer(self(state), max_steps));
   return 1;
}

int isDangerous(lua_State* state)
{
   const ScriptedEnemy& enemy = self(state);
   const Constants::Direction direction = toDirection(luaL_optinteger(state, 1, -1));
   lua_pushboolean(
      state, enemy.getWorld().isDangerous(enemy.getTileX() + directionX(direction), enemy.getTileY() + directionY(direction))
   );
   return 1;
}

int timer(lua_State* state)
{
   self(state).startTimer(static_cast<int32_t>(luaL_checkinteger(state, 1)), static_cast<int32_t>(luaL_checkinteger(state, 2)));
   return 0;
}

int setShield(lua_State* state)
{
   self(state).setShielded(lua_toboolean(state, 1) != 0);
   return 0;
}

int hitPoints(lua_State* state)
{
   lua_pushinteger(state, self(state).getHitPoints());
   return 1;
}

int teleport(lua_State* state)
{
   ScriptedEnemy& enemy = self(state);
   const auto target = enemy.getWorld().getRandomFreeTile(static_cast<int32_t>(luaL_optinteger(state, 1, 4)));

   if (target)
   {
      enemy.setPosition(static_cast<float>(target->x()) + 0.5f, static_cast<float>(target->y()) + 0.5f);
      enemy.setDirection(Constants::DirectionUnknown);
   }

   lua_pushboolean(state, target.has_value());
   return 1;
}

int spawn(lua_State* state)
{
   ScriptedEnemy& enemy = self(state);
   const auto type = typeArgument(state, 1);

   if (!type)
   {
      return 0;
   }

   lua_pushinteger(state, enemy.getWorld().spawnEnemy(*type, enemy.getX(), enemy.getY(), enemy.getId()));
   return 1;
}

int count(lua_State* state)
{
   const auto type = typeArgument(state, 1);
   lua_pushinteger(state, type ? self(state).getWorld().countEnemies(*type) : 0);
   return 1;
}

int fire(lua_State* state)
{
   ScriptedEnemy& enemy = self(state);
   enemy.getWorld().spitFire(enemy.getTileX(), enemy.getTileY(), static_cast<int32_t>(luaL_optinteger(state, 1, 4)));
   return 0;
}

int dropBomb(lua_State* state)
{
   ScriptedEnemy& enemy = self(state);
   enemy.getWorld().dropBomb(enemy.getTileX(), enemy.getTileY(), static_cast<int32_t>(luaL_optinteger(state, 1, 2)));
   return 0;
}

int logMessage(lua_State* state)
{
   qDebug("enemy %d: %s", self(state).getId(), luaL_checkstring(state, 1));
   return 0;
}

}  // namespace api

std::string readFile(const std::string& path)
{
   std::ifstream file(path, std::ios::binary);

   if (!file)
   {
      return {};
   }

   std::stringstream contents;
   contents << file.rdbuf();
   return contents.str();
}
}  // namespace

ScriptedEnemy::ScriptedEnemy(int16_t id, EnemyType type, EnemyWorld& world, std::string script_path)
    : Enemy(id, type, world), _script_path(std::move(script_path))
{
}

ScriptedEnemy::~ScriptedEnemy()
{
   if (_state)
   {
      lua_close(_state);
   }
}

void ScriptedEnemy::initialize()
{
   const std::string source = readFile(_script_path);

   if (source.empty())
   {
      qWarning("ScriptedEnemy: no script at %s", _script_path.c_str());
      return;
   }

   _state = luaL_newstate();
   luaL_openlibs(_state);

   // shared helpers are required by name from the same directory
   lua_getglobal(_state, "package");
   lua_pushstring(_state, (std::string(SCRIPT_DIRECTORY) + "?.lua").c_str());
   lua_setfield(_state, -2, "path");
   lua_pop(_state, 1);

   registerFunctions();

   if (luaL_loadbuffer(_state, source.data(), source.size(), _script_path.c_str()) != LUA_OK || lua_pcall(_state, 0, 0, 0) != LUA_OK)
   {
      qWarning("ScriptedEnemy: %s", lua_tostring(_state, -1));
      lua_pop(_state, 1);
      return;
   }

   _valid = true;

   readProperties();
   call("initialize", 0, 0);
}

bool ScriptedEnemy::isValid() const
{
   return _valid;
}

void ScriptedEnemy::registerFunctions()
{
   const std::array<std::pair<const char*, lua_CFunction>, 22> functions = {{
      {"direction", api::direction},
      {"set_direction", api::setDirection},
      {"speed", api::speed},
      {"set_speed", api::setSpeed},
      {"set_wall_pass", api::setWallPass},
      {"position", api::position},
      {"tile", api::tile},
      {"can_move", api::canMove},
      {"free_directions", api::freeDirections},
      {"random", api::randomNumber},
      {"nearest_player", api::nearestPlayer},
      {"direction_to_player", api::directionToPlayer},
      {"is_dangerous", api::isDangerous},
      {"timer", api::timer},
      {"set_shield", api::setShield},
      {"hit_points", api::hitPoints},
      {"teleport", api::teleport},
      {"spawn", api::spawn},
      {"count", api::count},
      {"fire", api::fire},
      {"drop_bomb", api::dropBomb},
      {"log", api::logMessage},
   }};

   // every function carries its enemy as upvalue, no lookup by state needed
   for (const auto& [name, function] : functions)
   {
      lua_pushlightuserdata(_state, this);
      lua_pushcclosure(_state, function, 1);
      lua_setglobal(_state, name);
   }

   const std::array<std::pair<const char*, int32_t>, 5> constants = {{
      {"NONE", Constants::DirectionUnknown},
      {"UP", Constants::DirectionUp},
      {"DOWN", Constants::DirectionDown},
      {"LEFT", Constants::DirectionLeft},
      {"RIGHT", Constants::DirectionRight},
   }};

   for (const auto& [name, value] : constants)
   {
      lua_pushinteger(_state, value);
      lua_setglobal(_state, name);
   }
}

void ScriptedEnemy::readProperties()
{
   lua_getglobal(_state, "properties");

   if (lua_istable(_state, -1))
   {
      lua_getfield(_state, -1, "speed");
      if (lua_isnumber(_state, -1))
      {
         setSpeed(static_cast<float>(lua_tonumber(_state, -1)));
      }
      lua_pop(_state, 1);

      lua_getfield(_state, -1, "wall_pass");
      setWallPass(lua_toboolean(_state, -1) != 0);
      lua_pop(_state, 1);

      lua_getfield(_state, -1, "hit_points");
      if (lua_isinteger(_state, -1))
      {
         setHitPoints(static_cast<int32_t>(lua_tointeger(_state, -1)));
      }
      lua_pop(_state, 1);

      lua_getfield(_state, -1, "points");
      if (lua_isinteger(_state, -1))
      {
         setPoints(static_cast<int32_t>(lua_tointeger(_state, -1)));
      }
      lua_pop(_state, 1);
   }

   lua_pop(_state, 1);
}

bool ScriptedEnemy::call(const char* function, int32_t nargs, int32_t nresults)
{
   if (!_valid)
   {
      if (_state)
      {
         lua_pop(_state, nargs);
      }

      return false;
   }

   lua_getglobal(_state, function);

   if (!lua_isfunction(_state, -1))
   {
      lua_pop(_state, nargs + 1);
      return false;
   }

   // the function goes below its arguments
   lua_insert(_state, -(nargs + 1));

   if (lua_pcall(_state, nargs, nresults, 0) != LUA_OK)
   {
      qWarning("ScriptedEnemy: %s: %s", function, lua_tostring(_state, -1));
      lua_pop(_state, 1);

      // a broken script stops thinking instead of flooding the log every tick
      _valid = false;
      return false;
   }

   return true;
}

Constants::Direction ScriptedEnemy::think(int32_t blocked)
{
   if (!_valid)
   {
      return Enemy::think(blocked);
   }

   lua_pushinteger(_state, blocked);

   if (!call("think", 1, 1))
   {
      return Enemy::think(blocked);
   }

   const Constants::Direction direction = lua_isinteger(_state, -1) ? toDirection(lua_tointeger(_state, -1)) : Constants::DirectionUnknown;
   lua_pop(_state, 1);
   return direction;
}

void ScriptedEnemy::tick(float dt)
{
   if (_valid)
   {
      lua_pushnumber(_state, dt);
      call("update", 1, 0);
   }
}

void ScriptedEnemy::timeout(int32_t id)
{
   if (_valid)
   {
      lua_pushinteger(_state, id);
      call("timeout", 1, 0);
   }
}

void ScriptedEnemy::hurt()
{
   if (_valid)
   {
      lua_pushinteger(_state, getHitPoints());
      call("hit", 1, 0);
   }
}

void ScriptedEnemy::died()
{
   if (_valid)
   {
      call("died", 0, 0);
   }
}

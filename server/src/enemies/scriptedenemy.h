#pragma once

#include "enemy.h"

#include <string>

struct lua_State;

//! an enemy whose behaviour lives in data/scripts/enemies/<name>.lua
class ScriptedEnemy : public Enemy
{
public:
   ScriptedEnemy(int16_t id, EnemyType type, EnemyWorld& world, std::string script_path);
   ~ScriptedEnemy() override;

   ScriptedEnemy(const ScriptedEnemy&) = delete;
   ScriptedEnemy& operator=(const ScriptedEnemy&) = delete;

   void initialize() override;

   //! the script could be loaded and run
   [[nodiscard]] bool isValid() const;

protected:
   Constants::Direction think(int32_t blocked) override;
   void tick(float dt) override;
   void timeout(int32_t id) override;
   void hurt() override;
   void died() override;

private:
   void registerFunctions();
   void readProperties();

   //! calls a global script function if it exists, leaving nresults on the stack on success
   bool call(const char* function, int32_t nargs, int32_t nresults);

   std::string _script_path;
   lua_State* _state = nullptr;
   bool _valid = false;
};

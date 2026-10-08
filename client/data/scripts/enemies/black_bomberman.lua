-- black bomberman: the final boss, invincible behind his force field while his four henchmen live;
-- then he drops bombs, toggles the field, teleports and sprays fire
local common = require("common")

properties = {
   speed = 1.6,
   points = 50000,
   hit_points = 5,
   wall_pass = false,
}

local BOMB_TIMER = 1
local SHIELD_TIMER = 2
local TELEPORT_TIMER = 3
local second_phase = false

function initialize()
   set_shield(true)
end

function start_second_phase()
   second_phase = true
   set_shield(false)
   timer(2500, BOMB_TIMER)
   timer(5000, SHIELD_TIMER)
   timer(8000, TELEPORT_TIMER)
end

function update(dt)
   if not second_phase and count("bomberman") == 0 then
      start_second_phase()
   end
end

function timeout(id)
   if id == BOMB_TIMER then
      drop_bomb(3)
      timer(3000 + random(1500), BOMB_TIMER)
   elseif id == SHIELD_TIMER then
      local shielded = random(2) == 0
      set_shield(shielded)
      timer(shielded and 2500 or 4000, SHIELD_TIMER)
   elseif id == TELEPORT_TIMER then
      if teleport(4) then
         fire(4)
      end
      timer(9000 + random(3000), TELEPORT_TIMER)
   end
end

function think(blocked)
   return common.wander(common.avoid_danger(blocked), 35)
end

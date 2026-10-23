-- spidfire (setsutore): boss of round 7, wanders and periodically stops behind a force field to
-- spray fire four tiles into every direction
local common = require("common")

properties = {
   speed = 1.5,
   points = 30000,
   hit_points = 3,
   wall_pass = false,
}

local STOP_TIMER = 1
local FIRE_TIMER = 2
local GO_TIMER = 3
local stopped = false

function initialize()
   timer(4000 + random(2000), STOP_TIMER)
end

function timeout(id)
   if id == STOP_TIMER then
      stopped = true
      set_shield(true)
      timer(800, FIRE_TIMER)
   elseif id == FIRE_TIMER then
      fire(4)
      timer(900, GO_TIMER)
   elseif id == GO_TIMER then
      stopped = false
      set_shield(false)
      timer(4000 + random(2000), STOP_TIMER)
   end
end

function think(blocked)
   if stopped then
      return NONE
   end
   return common.wander(blocked, 35)
end

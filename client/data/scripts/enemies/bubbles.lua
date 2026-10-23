-- bubbles: boss of rounds 2 and 5, wanders around deploying pink clouds that drift after bomberman;
-- after a while it starts the pursuit itself
local common = require("common")

properties = {
   speed = 1.3,
   points = 20000,
   hit_points = 2,
   wall_pass = false,
}

local CLOUD_TIMER = 1
local PURSUIT_TIMER = 2
local MAX_CLOUDS = 4
local pursuing = false

function initialize()
   timer(2500, CLOUD_TIMER)
   timer(15000, PURSUIT_TIMER)
end

function timeout(id)
   if id == CLOUD_TIMER then
      if count("cloud") < MAX_CLOUDS then
         spawn("cloud")
      end
      timer(4000, CLOUD_TIMER)
   elseif id == PURSUIT_TIMER then
      pursuing = true
      set_speed(1.8)
   end
end

function think(blocked)
   if pursuing then
      return common.chase(blocked, 64, 20)
   end
   return common.wander(blocked, 35)
end

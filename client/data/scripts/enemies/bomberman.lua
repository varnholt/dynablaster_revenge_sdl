-- one of black bomberman's four henchmen: wanders, closes in when bomberman is near and now and
-- then bursts into flames shooting fire into all four directions
local common = require("common")

properties = {
   speed = 1.6,
   points = 1000,
   hit_points = 2,
   wall_pass = false,
}

local FIRE_TIMER = 1

function initialize()
   timer(5000 + random(3000), FIRE_TIMER)
end

function timeout(id)
   if id == FIRE_TIMER then
      fire(3)
      timer(6000 + random(3000), FIRE_TIMER)
   end
end

function think(blocked)
   return common.chase(common.avoid_danger(blocked), 4, 30)
end

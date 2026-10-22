-- henchman: one of the shadow bomber's four henchmen, wanders, closes in when the player is near and
-- now and then bursts into flames shooting fire into all four directions
local common = require("common")

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

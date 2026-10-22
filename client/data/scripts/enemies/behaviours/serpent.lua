-- serpent: boss of rounds 1 and 3, pursues the player alternating between a slow and a fast pace
local common = require("common")

local PACE_TIMER = 1
local fast = false

function initialize()
   timer(3000, PACE_TIMER)
end

function timeout(id)
   if id == PACE_TIMER then
      fast = not fast
      set_speed(fast and properties.speed * 2 or properties.speed)
      timer(fast and 2000 or 3000, PACE_TIMER)
   end
end

function think(blocked)
   return common.chase(blocked, 64, 25)
end

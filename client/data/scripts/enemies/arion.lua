-- arion: boss of rounds 1 and 3, pursues bomberman alternating between a slow and a fast pace
local common = require("common")

properties = {
   speed = 1.2,
   points = 20000,
   hit_points = 5,
   wall_pass = false,
}

local PACE_TIMER = 1
local fast = false

function initialize()
   timer(3000, PACE_TIMER)
end

function timeout(id)
   if id == PACE_TIMER then
      fast = not fast
      set_speed(fast and 2.4 or 1.2)
      timer(fast and 2000 or 3000, PACE_TIMER)
   end
end

function think(blocked)
   return common.chase(blocked, 64, 25)
end

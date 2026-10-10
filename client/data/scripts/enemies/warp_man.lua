-- warp man: boss of rounds 4 and 6, wanders, teleports now and then and breeds one-eyed blobs
local common = require("common")

properties = {
   speed = 1.4,
   points = 30000,
   hit_points = 3,
   wall_pass = false,
}

local TELEPORT_TIMER = 1
local CHILD_TIMER = 2
local MAX_CHILDREN = 3

function initialize()
   timer(6000, TELEPORT_TIMER)
   timer(4000, CHILD_TIMER)
end

function timeout(id)
   if id == TELEPORT_TIMER then
      teleport(5)
      timer(6000 + random(3000), TELEPORT_TIMER)
   elseif id == CHILD_TIMER then
      if count("warp_man_child") < MAX_CHILDREN then
         spawn("warp_man_child")
      end
      timer(5000, CHILD_TIMER)
   end
end

function think(blocked)
   return common.wander(blocked, 35)
end

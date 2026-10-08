-- telpio: passes soft blocks, aimless and faster than boyon
local common = require("common")

properties = {
   speed = 1.9,
   points = 1000,
   hit_points = 1,
   wall_pass = true,
}

function think(blocked)
   return common.wander(blocked, 40)
end

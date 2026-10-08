-- ballom: moves in a repetitive manner, only turns when hitting a wall
local common = require("common")

properties = {
   speed = 1.0,
   points = 100,
   hit_points = 1,
   wall_pass = false,
}

function think(blocked)
   return common.straight(blocked)
end

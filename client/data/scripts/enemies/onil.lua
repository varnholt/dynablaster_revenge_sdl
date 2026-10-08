-- onil: moves quickly, sometimes turning or reversing
local common = require("common")

properties = {
   speed = 2.0,
   points = 400,
   hit_points = 1,
   wall_pass = false,
}

function think(blocked)
   return common.wander(blocked, 25)
end

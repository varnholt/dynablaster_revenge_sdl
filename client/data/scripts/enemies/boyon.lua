-- boyon: moves aimlessly, willing to change direction at random
local common = require("common")

properties = {
   speed = 1.5,
   points = 1000,
   hit_points = 1,
   wall_pass = false,
}

function think(blocked)
   return common.wander(blocked, 25)
end

-- pomori: floats through soft blocks, moves aimlessly
local common = require("common")

properties = {
   speed = 1.5,
   points = 800,
   hit_points = 1,
   wall_pass = true,
}

function think(blocked)
   return common.wander(blocked, 25)
end

-- pontan: passes soft blocks and relentlessly pursues bomberman at top speed
local common = require("common")

properties = {
   speed = 2.8,
   points = 200,
   hit_points = 1,
   wall_pass = true,
}

function think(blocked)
   return common.chase(blocked, 64, 25)
end

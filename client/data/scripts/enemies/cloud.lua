-- cloud: bubbles' pink cloud, slowly drifting after bomberman
local common = require("common")

properties = {
   speed = 0.8,
   points = 100,
   hit_points = 1,
   wall_pass = true,
}

function think(blocked)
   return common.chase(blocked, 64, 25)
end

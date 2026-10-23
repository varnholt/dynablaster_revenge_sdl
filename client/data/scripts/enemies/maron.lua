-- maron: turns or reverses when it bumps into walls or bombs
local common = require("common")

properties = {
   speed = 1.6,
   points = 100,
   hit_points = 1,
   wall_pass = false,
}

function think(blocked)
   return common.straight(blocked)
end

-- warp_man_child: warp man's one-eyed blob, chases bomberman, 3 hits
local common = require("common")

properties = {
   speed = 1.2,
   points = 100,
   hit_points = 3,
   wall_pass = false,
}

function think(blocked)
   return common.chase(blocked, 16, 25)
end

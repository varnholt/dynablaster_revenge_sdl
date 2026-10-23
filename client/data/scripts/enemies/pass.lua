-- pass: pursues bomberman at fast speed and stays away from bombs
local common = require("common")

properties = {
   speed = 2.3,
   points = 2000,
   hit_points = 1,
   wall_pass = false,
}

function think(blocked)
   return common.chase(common.avoid_danger(blocked), 12, 25)
end

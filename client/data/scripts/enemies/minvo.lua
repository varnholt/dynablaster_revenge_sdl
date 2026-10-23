-- minvo: behaves like ballom
local common = require("common")

properties = {
   speed = 1.5,
   points = 100,
   hit_points = 1,
   wall_pass = false,
}

function think(blocked)
   return common.straight(blocked)
end

-- hunt: heads for the nearest player once it's within reach, roams otherwise
local common = require("common")

function think(blocked)
   if properties.avoid_danger then
      blocked = common.avoid_danger(blocked)
   end
   return common.chase(blocked, properties.reach or 64, properties.turn_chance or 25)
end

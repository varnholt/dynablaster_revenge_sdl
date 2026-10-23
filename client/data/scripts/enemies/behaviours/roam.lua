-- roam: wanders aimlessly, turning at crossings now and then
local common = require("common")

function think(blocked)
   return common.wander(blocked, properties.turn_chance or 25)
end

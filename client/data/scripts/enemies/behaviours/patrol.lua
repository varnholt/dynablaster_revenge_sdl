-- patrol: walks straight on and only turns when it bumps into something
local common = require("common")

function think(blocked)
   return common.straight(blocked)
end

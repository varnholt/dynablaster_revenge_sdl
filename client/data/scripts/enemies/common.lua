-- movement helpers shared by the enemy scripts
--
-- an enemy script sets a global 'properties' table and may define:
--   initialize(), think(blocked) -> direction, update(dt), timeout(id), hit(hit_points), died()
-- think() runs whenever the enemy reaches a tile centre; 'blocked' has bit (1 << direction) set for
-- every direction that can't be walked. returning NONE stops the enemy until it thinks again.

local common = {}

function common.is_blocked(blocked, direction)
   return direction == NONE or ((blocked >> direction) & 1) == 1
end

function common.opposite(direction)
   if direction == UP then return DOWN end
   if direction == DOWN then return UP end
   if direction == LEFT then return RIGHT end
   if direction == RIGHT then return LEFT end
   return NONE
end

function common.free(blocked)
   local result = {}
   for _, direction in ipairs({UP, DOWN, LEFT, RIGHT}) do
      if not common.is_blocked(blocked, direction) then
         result[#result + 1] = direction
      end
   end
   return result
end

function common.pick(list)
   if #list == 0 then
      return NONE
   end
   return list[random(#list) + 1]
end

-- prefers not to turn back, unless it's the only way
function common.pick_forward(blocked)
   local back = common.opposite(direction())
   local options = {}
   for _, candidate in ipairs(common.free(blocked)) do
      if candidate ~= back then
         options[#options + 1] = candidate
      end
   end
   if #options == 0 then
      return common.pick(common.free(blocked))
   end
   return common.pick(options)
end

-- walks straight on and only turns when it bumps into something
function common.straight(blocked)
   local current = direction()
   if not common.is_blocked(blocked, current) then
      return current
   end
   return common.pick(common.free(blocked))
end

-- walks straight on but turns at crossings now and then
function common.wander(blocked, turn_chance)
   local current = direction()
   if not common.is_blocked(blocked, current) and random(100) >= turn_chance then
      return current
   end
   return common.pick_forward(blocked)
end

-- heads for the nearest player when one is within reach (in tiles walked)
function common.chase(blocked, reach, turn_chance)
   local towards = direction_to_player(reach)
   if towards ~= NONE and not common.is_blocked(blocked, towards) then
      return towards
   end
   return common.wander(blocked, turn_chance)
end

-- treats tiles a bomb is about to burn as walls
function common.avoid_danger(blocked)
   local result = blocked
   for _, direction in ipairs({UP, DOWN, LEFT, RIGHT}) do
      if is_dangerous(direction) then
         result = result | (1 << direction)
      end
   end
   return result
end

return common

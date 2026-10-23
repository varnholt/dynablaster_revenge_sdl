-- every story mode enemy: which behaviour drives it and how it's tuned
--
-- the entry becomes the enemy's global 'properties' table before behaviours/<behaviour>.lua runs, so a
-- behaviour reads its parameters from there:
--   speed, points, hit_points, wall_pass   read by the engine for every enemy
--   turn_chance                            roam, hunt: percent chance to turn at a crossing
--   reach                                  hunt: how far (in tiles walked) the player is noticed
--   avoid_danger                           hunt: treats tiles a bomb is about to burn as walls

return {
   -- moves in a repetitive manner, only turns when hitting a wall
   ballom = { behaviour = "patrol", speed = 1.0, points = 100, hit_points = 1 },
   boma = { behaviour = "patrol", speed = 1.5, points = 100, hit_points = 1 },
   flapper = { behaviour = "patrol", speed = 1.5, points = 100, hit_points = 1 },
   minvo = { behaviour = "patrol", speed = 1.5, points = 100, hit_points = 1 },
   nagachamu = { behaviour = "patrol", speed = 1.5, points = 100, hit_points = 1 },
   ojin = { behaviour = "patrol", speed = 1.5, points = 400, hit_points = 1 },
   maron = { behaviour = "patrol", speed = 1.6, points = 100, hit_points = 1 },

   -- moves aimlessly, willing to change direction at random
   boyon = { behaviour = "roam", speed = 1.5, points = 1000, hit_points = 1, turn_chance = 25 },
   buffer = { behaviour = "roam", speed = 1.5, points = 400, hit_points = 1, turn_chance = 25 },
   gacha = { behaviour = "roam", speed = 1.5, points = 100, hit_points = 1, turn_chance = 25 },
   korisuke = { behaviour = "roam", speed = 1.0, points = 400, hit_points = 1, turn_chance = 25 },
   sashakin = { behaviour = "roam", speed = 1.5, points = 400, hit_points = 1, turn_chance = 25 },
   uotan = { behaviour = "roam", speed = 1.5, points = 400, hit_points = 1, turn_chance = 25 },
   onil = { behaviour = "roam", speed = 2.0, points = 400, hit_points = 1, turn_chance = 25 },
   ekutopu = { behaviour = "roam", speed = 1.0, points = 400, hit_points = 1, turn_chance = 50 },
   pomori = { behaviour = "roam", speed = 1.5, points = 800, hit_points = 1, turn_chance = 25, wall_pass = true },
   telpio = { behaviour = "roam", speed = 1.9, points = 1000, hit_points = 1, turn_chance = 40, wall_pass = true },

   -- goes after the player
   pass = { behaviour = "hunt", speed = 2.3, points = 2000, hit_points = 1, reach = 12, turn_chance = 25, avoid_danger = true },
   pontan = { behaviour = "hunt", speed = 2.8, points = 200, hit_points = 1, reach = 64, turn_chance = 25, wall_pass = true },
   -- bubbles' pink cloud, slowly drifting after the player
   cloud = { behaviour = "hunt", speed = 0.8, points = 100, hit_points = 1, reach = 64, turn_chance = 25, wall_pass = true },
   -- warp man's one-eyed blob
   warp_man_child = { behaviour = "hunt", speed = 1.2, points = 100, hit_points = 3, reach = 16, turn_chance = 25 },

   -- bosses have a behaviour of their own
   arion = { behaviour = "arion", speed = 1.2, points = 20000, hit_points = 5 },
   bubbles = { behaviour = "bubbles", speed = 1.3, points = 20000, hit_points = 2 },
   warp_man = { behaviour = "warp_man", speed = 1.4, points = 30000, hit_points = 3 },
   spidfire = { behaviour = "spidfire", speed = 1.5, points = 30000, hit_points = 3 },
   black_bomberman = { behaviour = "black_bomberman", speed = 1.6, points = 50000, hit_points = 5 },
   bomberman = { behaviour = "bomberman", speed = 1.6, points = 1000, hit_points = 2 },
}

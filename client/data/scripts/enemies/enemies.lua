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
   balloon = { behaviour = "patrol", speed = 1.0, points = 100, hit_points = 1 },
   frog = { behaviour = "patrol", speed = 1.5, points = 100, hit_points = 1 },
   hothead = { behaviour = "patrol", speed = 1.5, points = 100, hit_points = 1 },
   grinner = { behaviour = "patrol", speed = 1.5, points = 100, hit_points = 1 },
   statue = { behaviour = "patrol", speed = 1.5, points = 100, hit_points = 1 },
   chomper = { behaviour = "patrol", speed = 1.5, points = 400, hit_points = 1 },
   sprout = { behaviour = "patrol", speed = 1.6, points = 100, hit_points = 1 },

   -- moves aimlessly, willing to change direction at random
   droplet = { behaviour = "roam", speed = 1.5, points = 1000, hit_points = 1, turn_chance = 25 },
   gourd = { behaviour = "roam", speed = 1.5, points = 400, hit_points = 1, turn_chance = 25 },
   croc = { behaviour = "roam", speed = 1.5, points = 100, hit_points = 1, turn_chance = 25 },
   spinner = { behaviour = "roam", speed = 1.0, points = 400, hit_points = 1, turn_chance = 25 },
   jester = { behaviour = "roam", speed = 1.5, points = 400, hit_points = 1, turn_chance = 25 },
   blob = { behaviour = "roam", speed = 1.5, points = 400, hit_points = 1, turn_chance = 25 },
   onion = { behaviour = "roam", speed = 2.0, points = 400, hit_points = 1, turn_chance = 25 },
   octopus = { behaviour = "roam", speed = 1.0, points = 400, hit_points = 1, turn_chance = 50 },
   imp = { behaviour = "roam", speed = 1.5, points = 800, hit_points = 1, turn_chance = 25, wall_pass = true },
   totem = { behaviour = "roam", speed = 1.9, points = 1000, hit_points = 1, turn_chance = 40, wall_pass = true },

   -- goes after the player
   tiger = { behaviour = "hunt", speed = 2.3, points = 2000, hit_points = 1, reach = 12, turn_chance = 25, avoid_danger = true },
   coin = { behaviour = "hunt", speed = 2.8, points = 200, hit_points = 1, reach = 64, turn_chance = 25, wall_pass = true },
   -- the puffer's pink cloud, slowly drifting after the player
   cloud = { behaviour = "hunt", speed = 0.8, points = 100, hit_points = 1, reach = 64, turn_chance = 25, wall_pass = true },
   -- the cyclops' offspring
   eyeball = { behaviour = "hunt", speed = 1.2, points = 100, hit_points = 3, reach = 16, turn_chance = 25 },

   -- bosses have a behaviour of their own
   serpent = { behaviour = "serpent", speed = 1.2, points = 20000, hit_points = 5 },
   puffer = { behaviour = "puffer", speed = 1.3, points = 20000, hit_points = 2 },
   cyclops = { behaviour = "cyclops", speed = 1.4, points = 30000, hit_points = 3 },
   crab = { behaviour = "crab", speed = 1.5, points = 30000, hit_points = 3 },
   shadow_bomber = { behaviour = "shadow_bomber", speed = 1.6, points = 50000, hit_points = 5 },
   henchman = { behaviour = "henchman", speed = 1.6, points = 1000, hit_points = 2 },
}

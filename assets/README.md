# Asset sources

What the game's 3D assets are made from: one folder per set of assets, each with a `build_all.py` that rebuilds
them from scratch, headless, and installs them into `client/data`. The scripts are the source: nothing in here is
edited by hand in Blender and saved, so a change is a change to a script and a rerun.

Blender is found via the `BLENDER` environment variable or in its default install folder
(`C:\Program Files\Blender Foundation\Blender <version>\blender.exe`). Every folder keeps its intermediate files in
an `out/` folder of its own, which git ignores.

| Folder | Builds | Into | Takes |
| --- | --- | --- | --- |
| `intro/` | the story mode's intro: the cast, the lab, the mountains, the sky, the timeline | `client/data/game/story/intro/` | 20 s |
| `enemies/` | the story mode's enemies and bosses, rigged with idle, walk and die cycles | `client/data/game/enemies/` | 2 min |
| `story-worlds/` | per world of the story mode the floor, border and the dioramas around the stages | `client/data/game/story/world<n>/` | 10 min |
| `level-pirate/` | the pirate ship level: the ship baked by day and by night, the Caribbean around it, its cameras, block skins | `client/data/level-pirate/` | 2.5 min |
| `hud/` | `make_paused.py`, the story mode's PAUSED overlay (`client/data/menus/paused.psd`), needs the game's font | `client/data/menus/` | seconds |

`python assets/<folder>/build_all.py` builds one set; `enemies` and `story-worlds` take names or numbers to build
only some.

## Reviewing a model

`review.py` renders models side by side (textured from four sides, then wireframes) and measures what makes a mesh
look made by hand: triangles, loose pieces, triangles cutting through each other, how evenly edges and texture
resolution are spread. Compare a new model against the original player:

    blender --background --factory-startup --python assets/review.py -- sheet.png assets/intro/out/player.glb <model.glb>

`intro/refine.py` turns a character built from primitives into such a model: the parts fused into one surface per
limb, laid out in even quads, unwrapped along the colour borders, colours, painted details and ambient occlusion
baked. The princess and the doctor go through it.

## Detail up close

`detail.py` makes small seamless images that the game's glTF shader tiles in world space over a material's colour,
and per material a mask of where each shows; `embed()` puts them into a finished `.glb` (the material's extras name
them). Three sets: nature (grass tufts, soil, cracked rock, pebbles) for worlds 1–5, castle (worn tiles, brick
grit, brushed metal, stone) for 6–8, ship (wood grain, canvas, old paint, metal) for the pirate ship. Each
material's mask comes from its own colours.

## How exact a rebuild is

The geometry comes out the same on every run. Baked textures can differ a little:

- ambient occlusion is sampled, so the enemies' occlusion maps vary by a few levels between runs;
- the dioramas of worlds 1, 6 and 7 have so many props that Blender packs their texture atlas differently each run;
  texture and uvs change together, the dioramas look the same.

So a rebuild leaves files changed that look as they did; commit only those you meant to change.

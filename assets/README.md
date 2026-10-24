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

## How exact a rebuild is

The geometry comes out the same on every run. Baked textures can differ a little:

- ambient occlusion is sampled, so the enemies' occlusion maps vary by a few levels between runs;
- the dioramas of worlds 1, 6 and 7 have so many props that Blender packs their texture atlas differently each run;
  texture and uvs change together, the dioramas look the same.

So a rebuild leaves files changed that look as they did; commit only those you meant to change.

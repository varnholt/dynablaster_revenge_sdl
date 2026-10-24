# Asset sources

What the game's 3D assets are made from: one folder per set of assets, each with a `build_all.py` that rebuilds
them from scratch, headless, and installs them into `client/data`. The scripts are the source: nothing in here is
edited by hand in Blender and saved, so a change is a change to a script and a rerun.

Blender is found via the `BLENDER` environment variable or in its default install folder
(`C:\Program Files\Blender Foundation\Blender <version>\blender.exe`). Every folder keeps its intermediate files in
an `out/` folder of its own, which git ignores.

| Folder | Builds | Into |
| --- | --- | --- |
| `intro/` | the story mode's intro: the cast, the lab, the mountains, the timeline | `client/data/game/story/intro/` |

`python assets/<folder>/build_all.py` builds one set.

# Effect lab scenario (shared by old Qt client and new SDL client)

Goal: render the same effect in both clients under identical, deterministic conditions and
capture PNGs at identical times, so they can be composited side by side.

## Window / clock
- Client area 1024x576.
- Fixed clock: frame n (starting at 0) has GlobalTime = n / 60 seconds, no wall clock anywhere.
- GameDrawable::animate(t) is called each frame with t = (n / 60 * 1000) * 0.0625
  (the "real ms * 0.0625" convention of the original BombermanView / new main.cpp).
- Every frame: update clock -> timers -> gameDrawable.animate(t) -> gameDrawable.paintGL() -> swap.

## Scene setup (frame 0, before the first paint)
1. setPlayfieldSize(13, 11)
2. setPlayfieldScale(1.0, 1.0)
3. loadLevel(<castle level name as the server sends it>)
4. setPlayerId(0)
5. addPlayer(0, "lab", ColorWhite)      // local player
6. addPlayer(1, "bot", ColorRed)        // second player, for effects that are not local-only
7. setPlayerPosition(0, 6.5, 5.5, 0.0)
8. setPlayerPosition(1, 4.5, 5.5, 0.0)
Camera: GameDrawable's default (follows the local player). For that to behave like a real game,
BombermanClient must also hold the game/player state it normally gets from the server:
- a GameInformation with id 0, Dimension13x11, level name = castle, pushed into
  BombermanClient's game list, and setGameId(0) - otherwise getDimensions() returns 0x0.
- PlayerInfo for id 0 (ColorWhite, pos 6.5,5.5, angle 0) and id 1 (ColorRed, 4.5,5.5) in
  BombermanClient's player-info map; setCurrentPlayerInfo(<id 0 info>); setPlayerId(0).
Do NOT call BombermanClient::initialize() or connect anywhere.

## Trigger
At frame 60 (t = 1.0 s) fire the effect's trigger call once.

## Effects (name -> trigger)
- baseline   : nothing
- mushroom   : playerInfected(0, SkullMushroom, -1, 6, 5)
- invisible  : playerInfected(0, SkullInvisible, -1, 6, 5)
- invincible : playerInfected(0, SkullInvincible, -1, 6, 5)
- infected   : playerInfected(0, SkullSlow, -1, 6, 5)
- startalers : extraRemoved(6, 5, false, ExtraBomb, 0)
- death      : removePlayer(0)
- detonation : addDetonation(6, 5, 2, 2, 3, 3, 1.0) (new client only so far)
- fuse       : createMapItem(new BombMapItem(0, 2, 100, 8, 5)) - a bomb two tiles right of the local
               player; the lab keeps it alive (never removed) for all captures
- extrareveal : createMapItem(new ExtraMapItem(101, ExtraFlame, 8, 5)) - an extra appearing (reveal
               frustum); the lab keeps it alive for all captures
- extradestroy : extraRemoved(8, 5, true, ExtraFlame, -1) - an extra destroyed by a flame

## Captures
Frames 60 + {6, 30, 60, 120, 180, 300} (= +100, +500, +1000, +2000, +3000, +5000 ms after the
trigger). Read the back buffer right before swap. File name: `<effect>_<ms>.png`, e.g.
`mushroom_500.png`, written to `D:\git\effect-lab\old\` or `D:\git\effect-lab\new\`.
snow: no trigger (like baseline) but additionally captures at frames 60 + {600, 900, 1200}
(+10000/15000/20000 ms); flakes only become visible after the countdown and a respawn.
extrareveal/extradestroy additionally capture at frames 60 + {3, 9, 15, 21} (+50/150/250/350 ms).
Process exits after the last capture.

## Invocation
Level: castle by default; `--level=level-mansion` (new) / `DYNA_EFFECT_LAB_LEVEL=level-mansion` (old)
replaces the level directory name in both the GameInformation and loadLevel(). Write each
level's captures to its own dir, e.g. `D:\git\effect-lab\old-mansion\` / `new-mansion\`.

Environment variable `DYNA_EFFECT_LAB=<effect>` (old client) / harness flag
`--effect=<effect>` (new client). Output dir via `DYNA_EFFECT_LAB_OUT` / `--out=`.

# Dynablaster Revenge

[![build](https://github.com/varnholt/dynablaster_revenge_sdl/actions/workflows/build.yml/badge.svg)](https://github.com/varnholt/dynablaster_revenge_sdl/actions/workflows/build.yml)
[![Switch homebrew](https://github.com/varnholt/dynablaster_revenge_sdl/actions/workflows/switch.yml/badge.svg)](https://github.com/varnholt/dynablaster_revenge_sdl/actions/workflows/switch.yml)

_Dynablaster Revenge_ is a remake of the game Dynablaster, released by Hudson Soft in 1991. The
goal of this remake is to keep the original game-play as untouched as possible while adding
networked multiplayer and real-time 3D rendering. If you're not yet familiar with the original
game: bomb all other players off the screen. Collect flame extras to increase your bomb radius,
bomb extras for more bombs, and use clever chain reactions to corner your opponents.

This is the Qt-free rewrite of the project: a native SDL3/OpenGL ES 3.0 client and a dedicated
game server, both written in C++23 with no Qt dependency.

# Get a Build

Every push to `main` is built for all five platforms. These links always give you the newest
successful build and need no GitHub account:

|Platform|Download|
|-|-|
|Windows|[dynablaster-windows.zip](https://nightly.link/varnholt/dynablaster_revenge_sdl/workflows/build/main/dynablaster-windows.zip)|
|Linux|[dynablaster-linux.zip](https://nightly.link/varnholt/dynablaster_revenge_sdl/workflows/build/main/dynablaster-linux.zip)|
|macOS|[dynablaster-macos.zip](https://nightly.link/varnholt/dynablaster_revenge_sdl/workflows/build/main/dynablaster-macos.zip)|
|Web|[dynablaster-wasm.zip](https://nightly.link/varnholt/dynablaster_revenge_sdl/workflows/build/main/dynablaster-wasm.zip)|
|Nintendo Switch (homebrew)|[dynablaster-switch.zip](https://nightly.link/varnholt/dynablaster_revenge_sdl/workflows/switch/main/dynablaster-switch.zip)|

The desktop archives contain `dynablaster_revenge`/`dynablaster_revenge.exe` next to its `data/`
directory - run it from that folder. The web archive holds the Emscripten output (`.html`/`.js`/
`.wasm`/`.data`) and needs to be served over HTTP, not opened as a local file. Note: browser
multiplayer doesn't work yet - the web build reaches the main menu only, since SDL3_net has no
WebSocket backend to talk to a real game server from inside a browser sandbox. The Switch archive
holds a single `.nro`; see [docs/switch.md](docs/switch.md) for how to install it.

# Project layout

- `client/` - the game client (SDL3 windowing/input, OpenGL ES 3.0 rendering, C++23)
- `server/` - the dedicated game server (SDL3_net for networking)
- `shared/` - wire protocol and game-object code shared by client and server
- `ai/` - bot AI (A* pathfinding + bot behavior state machine)

# Dependencies

- [SDL3](https://github.com/libsdl-org/SDL) - fetched and built automatically by CMake
- [SDL3_net](https://github.com/libsdl-org/SDL_net) - fetched and built automatically by CMake
- [minimp3](https://github.com/lieff/minimp3) - header-only MP3 decoder, fetched automatically
- a C++23 compiler (MSVC 2022, gcc 13+, or clang 16+)
- CMake 3.20+

All third-party dependencies are pulled in via CMake's `FetchContent`, so no manual dependency
installation is required beyond a working C++ toolchain and CMake.

# Controls

Keyboard: arrow keys move, space places a bomb, `[` / `]` zoom, Tab shows player names, Escape
leaves the game, F10 ends the round for its owner.

Controllers can be plugged in at any time. In menus, the D-pad or stick moves the
focus from item to item, A/B/X/Y click; in a text field up/down cycle the letter and left/right
move the cursor. In game, the stick or D-pad moves, A/B/X/Y place a bomb and the shoulder
buttons zoom; the controller rumbles when you collect an extra or die. Any controller
SDL3 recognizes as a gamepad works.

More players can play on the same machine. With controllers connected, joining or creating a
game first shows the controls page: one column per player with its device, color and name. Each
controller steers its own column (left/right: color, up/down: move to another column, A: OK),
the keyboard does the same for its column; with the mouse the upper arrows pick the color and
the lower ones the device (incl. none). The first column joins as the main player, the others as
further players on their own connections. The setup is remembered per number of controllers in
`game.ini` (`[controls<n>]`), controllers are recognized by their hardware id. The camera keeps
all players of this machine in view.

# Building

## Client

```
cd client
cmake -S . -B build
cmake --build build --config Release
```

This produces `dynablaster_revenge` (the game) and `dynablaster_revenge_harness` (a diagnostic/test
binary supporting `--selftest`, `--menu`, `--click`, `--screenshot`, and other tooling flags).

## Nintendo Switch homebrew

The Switch `.nro` is built with a pinned devkitPro Docker toolchain via `build_switch.bat`. See
[docs/switch.md](docs/switch.md) for installing, controls, building, and emulator notes.

## Server

```
cd server
cmake -S . -B build
cmake --build build --config Release
```

This produces `dynablaster_server`, a standalone dedicated server.

On Windows the CMake-default generator is Visual Studio (MSVC); on Linux and macOS CMake will
pick your default C++ toolchain (Makefiles/Ninja).

# Requirements

To run the game you need a graphics driver with OpenGL ES 3.0 support.

# Help

## Extras

### Bomb
Pick up the bomb extra and you'll have one more bomb to place. This is one of the most simple
extras; with it and some practice you may learn tactics to drive your opponents into a corner.

### Flame
Collect the flame extra to create larger detonations. With each additional flame you'll be able
to reach one more space with your bombs. Always keep track on the number of bombs your opponents
have collected - it could save your life.

### Speedup
As the name suggests, your player's speed will increase with each speedup collected. Speedups
will make your player much more agile and let you reach extras before your opponents do.

### Kick
You'll either love or hate this one - kick extras enable you to kick bombs away until they reach
any obstacle like players, walls or other bombs. This extra will definitely mess up all your
opponent's tactics.

### Skulls
This extra actually consists of six separate ones. Once the skull extra is revealed you'll see a
rotating cube; each side of the cube represents a different effect when picked up. There's one
that will make your player drop bombs all the time, another one that will flip your controls, and
a third that will restrict your bombs to a single flame. The mushroom skull will make you quite
dizzy, but there's also cool stuff: invisibility and invincibility - yes, invincibility. By the
way, the skull extra is predictable - you'll soon find out how it works.

## Hotkeys

- `[F1]`, Open Hotkey overview
- `[F2]`, Mute music
- `[F3]`, Mute sound effects
- `[F4]`, Display your local IPs
- `[F10]`, End current game (game owner only)
- `[F11]`, Save game playback to disk (currently developer only)
- `[PageUp]`, Next track
- `[PageDown]`, Previous track
- `[Alt+Return]`, Toggle full screen

## Ingame hotkeys

- `[Return]`, Chat
- `[Escape]`, Rage quit (immediately leave the game)
- `[Tab]`, Show player names

## Multiplayer

Quick guide to local multiplayer setup:

1. Load the game, set the IP address to 127.0.0.1 and select 'Multi'. This will open a server on
   your PC. Create a new game and, in the lounge, add local players.

Quick guide to LAN multiplayer setup:

1. Set the IP address to 127.0.0.1, and select 'Multi'. This will open a server on your PC.
2. The other players then just have to connect to your IP address by entering your IP into the
   hostname field. If you don't know your IP just press F4 in the menus.

Quick guide to online multiplayer setup:

The best option here is to have a dedicated server (a Linux machine or a Windows machine
connected to a fast internet connection) you can run the server on. If you just want to use your
own connection to play over the internet, set up port forwarding from your router to your PC
(forward traffic on the game's port to your PC's LAN IP), host your server on 127.0.0.1 as you
would for a LAN game, then tell your friends your internet IP address so they can connect.

# Credits

## Core team
- mueslee (Matthias Varnholt), code & team lead
- hellfire (Christoph Grote), code
- dstar, artwork

## Level design
- dstar, Mansion concept & design
- Christopher Aldridge, space concept & design
- Sebastian Meckelmann, castle concept & design

## 3D
- xabotage (Daniel Phelps), character animation
- hellfire, main character design, UVs, game integration
- mueslee, space level lowpoly edit, UVs, game integration

## Music
- daxx, music & SFX
- svenzzon, music & SFX
- cold storage, music
- jco, music
- keito, music
- neoj1n (c.c.catch), music
- netpoet, music
- romeo knight, music
- skaven, music
- sunspire, music

## We thank
- alk, testing & feedback
- jan, testing & feedback
- fuxx, development infrastructure
- neoman, development infrastructure

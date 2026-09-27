# Dynablaster Revenge

[![build](https://github.com/varnholt/dynablaster_revenge_sdl/actions/workflows/build.yml/badge.svg)](https://github.com/varnholt/dynablaster_revenge_sdl/actions/workflows/build.yml)

_Dynablaster Revenge_ is a remake of the game Dynablaster, released by Hudson Soft in 1991. The
goal of this remake is to keep the original game-play as untouched as possible while adding
networked multiplayer and real-time 3D rendering. If you're not yet familiar with the original
game: bomb all other players off the screen. Collect flame extras to increase your bomb radius,
bomb extras for more bombs, and use clever chain reactions to corner your opponents.

This is the Qt-free rewrite of the project: a native SDL3/OpenGL ES 3.0 client and a dedicated
game server, both written in C++23 with no Qt dependency.

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

# Building

## Client

```
cd client
cmake -S . -B build
cmake --build build --config Release
```

This produces `dynablaster_revenge` (the game) and `dynablaster_revenge_harness` (a diagnostic/test
binary supporting `--selftest`, `--menu`, `--click`, `--screenshot`, and other tooling flags).

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

# Contact

All kind of bugreports, ideas and suggestions are greatly appreciated. In order to contact us,
please send a mail to:

- matthias`[dot]`varnholt`[at]`gmail`[dot]`com or
- hellfire`[at]`untergrund`[dot]`net or
- dstarx64`[at]`gmail`[dot]`com

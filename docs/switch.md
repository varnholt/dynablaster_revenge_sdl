# Nintendo Switch homebrew

This is a homebrew build for the original Switch. The newest `.nro` from `main` can be downloaded
as [dynablaster-switch.zip](https://nightly.link/varnholt/dynablaster_revenge_sdl/workflows/switch/main/dynablaster-switch.zip).

## Installing

Copy `dynablaster_revenge.nro` to the SD card's `/switch/` directory and launch it through the
homebrew menu in title takeover mode (hold R while starting a game), which provides the memory
needed by the assets. All game assets are embedded as RomFS.

Writable settings, host history, and diagnostics go to `sdmc:/switch/dynablaster_revenge/`;
`game.ini` is seeded there on the first launch.

## Controls

In menus, use the left stick or D-pad to move the cursor, A to click/drag, B to go
back, and X to open the software keyboard after selecting a text field. In game,
the stick/D-pad moves, A or B places a bomb, L/R zooms, ZL displays player names,
Minus leaves the game, and Plus ends the round for its owner. The initial port uses
one controller; single player with bots and network multiplayer use the existing
embedded server and SDL3_net protocol.

## Building

The Switch build uses a pinned devkitPro Docker toolchain, so it needs
Docker Desktop running with Linux containers, rather than a native devkitPro install:

```bat
build_switch.bat
```

This builds `client/build-switch/gcc14/dynablaster_revenge.nro` with
`devkitpro/devkita64:20240827` (GCC 14) and checksum-verified CMake 3.31.6.
GCC 15's unwinder uses GCS instructions unsupported by the tested emulator.

The SDL3 homebrew backend in `patches/switch-sdl3-backend.patch` is reused from
Deceptus and applied to its pinned SDL revision
`e205361fb67ff53868dbc333eb2c491e11ff1a51` from `vittorioromeo/SDL`.
The SDL zlib notices are preserved; the backend is a modified SDL distribution.

## Validation

To validate the NRO container, embedded assets, and linked platform backends:

```bat
docker run --rm -v "%CD%:/workspace" -w /workspace devkitpro/devkita64:20260219 python3 client/tests/test_switch_build.py
```

The validation container includes a host C compiler for the networking checks;
the NRO itself is built with the GCC 14 image above.

## Running in Ryujinx

For Ryujinx 1.3.2, enable network access so the client and embedded server can
connect through loopback, and use the normal tick scalar of 1. Numeric endpoints
are resolved locally. Socket setup verifies the nonblocking flag and corrects
the reversed `F_SETFL` behavior in this emulator when detected.

## Status

Runtime verification in stock Ryujinx 1.3.2 covers controller navigation, local
matches with three bots on Castle, Mansion, and Space, movement, bombs, and round
transitions. Handheld and docked rendering use the actual libnx framebuffer size.
Physical Switch hardware and multiplayer between separate consoles remain untested.

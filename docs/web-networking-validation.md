# Local validation — 2026-11-08

Environment: Windows x64, Visual Studio 18 / MSVC 19.51, Intel Graphics with
OpenGL ES 3.0, Emscripten 6.0.1, and Chromium 151.0.7922.71. Browser tests used
isolated headless Chromium with SwiftShader; no personal browser profile was used.

| Scenario | Result | Evidence |
| --- | --- | --- |
| Regular Windows executable, local embedded TCP server | Passed | Normal menu flow selected two bots, created/joined/started a game, moved, placed a bomb, left, and exited with status 0. Rendered screenshots and server logs retained. |
| Full WebAssembly game, server inside the browser | Passed | Normal menu flow created a battle with two bots. Arena, players, movement and bombs rendered; leaving returned to the menu. No native server or bridge was running for this scenario. |
| Full WebAssembly game over WebSocket/TCP | Passed | Normal menu flow played against two bots through the bridge and a native server. Screenshots, client console, bridge log and server log retained. |
| Two WebAssembly clients in actual Chromium, network server | Passed | Both clients logged in, created/joined/started a match, moved and placed a bomb, then disconnected and repeated in the same runtime. |
| Two WebAssembly clients plus embedded server in actual Chromium | Passed | The same repeated-match protocol checks ran with the real C++ server compiled into WebAssembly and no TCP listener. |
| Browser-local story mode | Passed | The normal Single flow skipped the intro, created the story, loaded the level, started, accepted controls and returned to the menu. |

The rendered browser checks reject runtime exceptions, invalid WebGL operations,
shader compilation failures and program-link failures. They exposed a missing
shader binding in the messaging overlay, which was corrected. Actual browser
shutdown also exposed static-destruction ordering in the embedded-server test
probe; the probe now destroys its clients and server before exiting the runtime.

The regular Windows session uses the existing `DYNABLASTER_DEV_SCRIPT` hook to
drive normal game input; it is not the diagnostic harness or protocol-only probe.
Screenshots were inspected for the Windows and browser arena, player/bomb states,
and return to the menu. The native Windows UI automation helper was unavailable,
so these were scripted application playtests rather than desktop mouse automation.

Reproduction commands and screenshot/log output paths are in
[browser networking](web-networking.md#verification). CI retains browser evidence
in the `browser-playtest-evidence` artifact, including on failure.

Audio playback quality, physical controllers and internet/WSS deployment were not
tested locally. A browser-local server serves only its own tab's players and bots;
online games currently use the native server and bridge. Cross-machine hosting
from a browser would require a peer transport and signaling service.

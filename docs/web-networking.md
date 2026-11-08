# Browser networking

The WebAssembly build can run the real server inside the tab for local games,
or connect through binary WebSockets for network games. Desktop and Switch clients keep
using SDL_net TCP; both carry the same length-prefixed game packets. A small
Python bridge converts WebSocket messages into a byte stream to one configured
native server. Browser and desktop players can join the same games.

```text
Browser game -- WebSocket --> bridge -- TCP :6300 --> Dynablaster server
Desktop game -------------------------- TCP :6300 --> same server
```

## Run locally

Build the browser client and serve it over HTTP:

```sh
python -m http.server 8000 --directory client/build-wasm
```

Open `http://localhost:8000/dynablaster_revenge.html`. With the default local host,
**Single** starts the story in the browser. **Multi** lets you create a local
battle; set the number of bots to two or more for a match against the computer.
The server, bots and players exchange ordinary game packets over in-memory
streams on the browser event loop. No native server, bridge, listening socket,
WebAssembly threads or isolation headers are needed for this mode.

This local server belongs to its tab. It does not accept connections from another
browser, tab or desktop client. Sharing a browser-hosted game over the internet
would need a separate peer transport and signaling service, such as WebRTC.

## Network games

Build the native server and browser client with the commands in the README.
Run the native server from its packaged directory so it finds `data/` and the
story scripts. Leave it running, then start the bridge in another terminal:

```sh
python -m venv .venv
# Activate .venv for your shell first.
python -m pip install -r web/requirements.txt
python web/bridge.py --origin http://localhost:8000
```

Serve the WebAssembly build over HTTP:

```sh
python -m http.server 8000 --directory client/build-wasm
```

Open:

```text
http://localhost:8000/dynablaster_revenge.html?server=ws://127.0.0.1:8081/
```

Choose a nickname, then **Multi** to create or join a game. Other browser tabs
using this endpoint and desktop clients connecting to the native server can join
the same game. Additional controller players and bots use their own connections
through the same endpoint. With an explicit `?server=`, **Single** also creates its
story game on that external server. The selected native server must remain running
for network games. Browser hosting does not open an inbound port on the player's device.

Without `?server=`, local host names (`127.0.0.1`, `localhost`, `::1`) select the
embedded server. A remote host selects `ws://<host>:8081/`, or `wss://<host>:8081/`
when the page uses HTTPS. The host field may also contain a complete WebSocket URL.
An explicit `?server=` overrides the host field for every client and bot. Use it
for deployed sites, custom ports and reverse-proxy paths. URL-encode the endpoint
when it contains its own query parameters.

The existing browser asset bundle contains the castle theme. Games using other
themes fall back to castle rendering. The download is large (roughly 514 MiB);
this change does not compress or stream the existing game assets.

## Public hosting

GitHub Pages can serve the static WebAssembly client, but cannot run the native
server or bridge. Run those on a host that supports persistent TCP/WebSocket
connections. Keep the bridge's fixed TCP target private to that host.

For an HTTPS frontend, use a TLS reverse proxy to expose the bridge as WSS:

```sh
python web/bridge.py --origin https://varnholt.github.io
```

Forward, for example, `wss://games.example.org/dynablaster` to
`http://127.0.0.1:8081`, preserving the WebSocket Upgrade and Origin headers.
Then use `?server=wss://games.example.org/dynablaster` on the frontend URL.
Repeat `--origin` to allow other frontend origins; it is an exact match including
scheme and port. The bridge binds only to loopback by default and never accepts
a target host from a WebSocket request. It disables compression, bounds incoming
messages and queues, and propagates disconnects in both directions.

Origin checks identify allowed browser pages; they are not player authentication.
The existing game protocol has no accounts or matchmaking service. A public game
server remains a separately operated service. No public endpoint is provisioned
by this PR.

## Transport interface

`ByteStream` exposes nonblocking reads and ordered writes without depending on
SDL. `ClientConnection` extends it with asynchronous connection state, error text
and an optional drain operation. `connectToHost` selects the platform backend:

- Native builds resolve the hostname and connect through SDL_net.
- WebAssembly builds use Emscripten's WebSocket API with binary frames and bounded
  send/receive buffers. Browser callbacks never parse game packets.
- Browser-local games wrap the existing in-memory stream in `ClientConnection`.
  The real server runs its normal timers on the browser event loop, and the same
  local factory supplies connections for additional players and bots.

The main client, additional local players and bot connections share this factory.
The server's accepted TCP sockets use `NetByteStream`. Packet reassembly stays
above these transports because a packet may span several reads or WebSocket frames.

## Verification

```sh
python -m unittest discover -s web/tests -v
cmake -S client -B client/build -DDYNABLASTER_NETWORK_TESTS=ON
cmake --build client/build --config Release --target dynablaster_network_test dynablaster_network_server
```

With the platform-specific executable paths:

```sh
python web/tests/run_network_game.py --server client/build/dynablaster_network_server --probe client/build/dynablaster_network_test
```

For WebAssembly, configure with `-DDYNABLASTER_NETWORK_TESTS=ON` and build
`dynablaster_network_test` as well as the game. Run the same test with Node 22+:

```sh
python web/tests/run_network_game.py --server client/build/dynablaster_network_server --probe client/build-wasm/dynablaster_network_test.js --node node
```

On Windows with a Visual Studio generator, executables are under `Release/` and
have an `.exe` suffix. The runner starts and stops only its own native server and
refuses to run when port 6300 is already in use. The probe uses the real native or
compiled WebAssembly connection backend: two clients log in, create and join a
game, synchronize, start, move and place a bomb. Reads and writes deliberately split
packets to exercise stream reassembly. Both clients then disconnect and repeat the
match in the same runtime to check connection and callback cleanup. CI also checks
malformed and unreachable WebSocket endpoints. The WebAssembly protocol test uses Node's
WebSocket API. `dynablaster_network_test.js --embedded` runs the same repeated
matches with the server compiled into WebAssembly.

For actual Chromium tests, install `web/tests/requirements-browser.txt` and run
`python -m playwright install chromium` (or set `CHROMIUM_PATH` to a test browser).

```sh
python web/tests/browser_playtest.py --build client/build-wasm --output build/browser-embedded-probe --probe
python web/tests/browser_playtest.py --build client/build-wasm --output build/browser-network-probe --probe --native-server client/build/dynablaster_network_server
python web/tests/browser_playtest.py --build client/build-wasm --output build/browser-local --steps web/tests/multiplayer.steps --expect "game 1 started"
python web/tests/browser_playtest.py --build client/build-wasm --output build/browser-network --steps web/tests/multiplayer.steps --native-server client/build/dynablaster_network_server --expect "game 1 started"
```

The rendered playtests use the normal menus to select two bots, create and join a
game, start, move, place a bomb and leave. They retain screenshots and console logs.
The browser probe checks two clients and repeated sessions inside a real browser.

On Windows, the normal desktop executable can run the same gameplay script:

```sh
python client/tests/playtest_windows.py --executable client/build/Release/dynablaster_revenge.exe --steps web/tests/multiplayer.steps --output build/windows-gameplay
```

That runner uses the game's existing `DYNABLASTER_DEV_SCRIPT` hook and records the
normal rendered game and embedded TCP server, rather than a diagnostic executable.
Audio playback quality and physical controllers still require a manual check.

Before deploying a public frontend, also play a match in two browser tabs and
with a desktop client, including leaving, reconnecting and a stopped bridge.

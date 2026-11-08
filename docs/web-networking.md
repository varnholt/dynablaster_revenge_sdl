# Browser networking

The browser build runs the real C++ server inside its tab for local games, or
connects directly to a native server over WebSockets for online games. Native
clients use TCP. Both carry the same game packets through the shared byte-stream
interface. SDL_net is the only networking library.

## Local browser games

Serve the WebAssembly build output with a static HTTP server, or deploy it to
GitHub Pages. Open `dynablaster_revenge.html` with the default local host.
**Single** starts the story inside the browser. **Multi** lets you create a local
battle; select two or more bots to play against the computer.

The server, bots and players exchange packets over in-memory streams on the
browser event loop. This mode needs no native server, listening socket,
WebAssembly threads or isolation headers. Its server belongs to that tab;
connections from other tabs or machines would require peer networking and a
signaling service, such as WebRTC.

## Network games

Build the native server and browser client using the README commands. Run the
server from its packaged directory so it finds `data/` and the story scripts:

```sh
dynablaster_server --websocket-port 8081 --origin http://localhost:8000
```

On Windows, use `dynablaster_server.exe`. The server accepts ordinary desktop
clients on TCP port 6300 and browser clients on WebSocket port 8081. Both use the
same games and players. The WebSocket listener is optional; without its command
line options, the server runs with its existing TCP behavior.

Serve the browser build at `http://localhost:8000` and open:

```text
http://localhost:8000/dynablaster_revenge.html?server=ws://127.0.0.1:8081/
```

Choose a nickname, then **Multi** to create or join a game. Other browser tabs
using this endpoint and desktop clients connecting to port 6300 can join the same
game. Additional local players and bots use the selected transport too. With an
explicit `?server=`, **Single** creates its story on the selected native server.

Repeat `--origin` to allow more frontend origins. Origins must match exactly,
including scheme and port. For example, `http://localhost:8000` and
`http://127.0.0.1:8000` are different origins. Requests with missing or unlisted
origins are rejected. This is an origin check, not player authentication.

Without `?server=`, local host names (`127.0.0.1`, `localhost`, `::1`) select the
embedded server. A remote host selects `ws://<host>:8081/`, or `wss://<host>:8081/`
when the frontend uses HTTPS. The host field can also contain a full WebSocket
URL. An explicit `?server=` overrides it for every player and bot. URL-encode the
endpoint if it contains its own query parameters.

## Public hosting

GitHub Pages serves the static browser client. Run the native game server on a
host that supports persistent connections. For an HTTPS frontend, terminate TLS
at a reverse proxy and forward WebSocket traffic to the server's listener:

```sh
dynablaster_server --websocket-port 8081 --origin https://varnholt.github.io
```

For example, forward `wss://games.example.org/dynablaster` to
`http://127.0.0.1:8081`, preserving Upgrade and Origin headers. Use
`?server=wss://games.example.org/dynablaster` on the frontend URL. The game server
handles the WebSocket upgrade and binary framing itself; the reverse proxy only
supplies TLS. Keep the unencrypted listener behind that proxy.

The browser build's large asset download (roughly 514 MiB) and fallback to castle
rendering for unavailable arena themes are unchanged.

## Transport interface

`ByteStream` provides nonblocking reads and ordered writes without depending on
SDL. `ClientConnection` adds asynchronous connection state, error text and an
optional drain operation. Native clients use SDL_net TCP, browser clients use
Emscripten's WebSocket API, and browser-local games wrap in-memory streams.

On the native server, accepted TCP sockets use `NetByteStream`; accepted browser
connections use `WebSocketByteStream`. The latter handles the HTTP upgrade,
masked binary and continuation frames, ping/pong and close frames. Handshakes
have a size limit and timeout, and message and outgoing queue sizes are bounded.
Packet reassembly stays above the transport because neither TCP reads nor
WebSocket frames necessarily align with game packets.

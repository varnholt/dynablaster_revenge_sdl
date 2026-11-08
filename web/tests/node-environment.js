// Run the compiled WebSocket backend with Node's standards-based WebSocket API.
// The game itself runs in a browser; this fixture exercises transport and protocol.
globalThis.location = {
  protocol: 'http:',
  search: '?server=' + encodeURIComponent(process.env.DYNABLASTER_TEST_SERVER || 'ws://127.0.0.1:8081/')
};

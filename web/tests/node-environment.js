// Run the compiled WebSocket backend with Node's standards-based WebSocket API.
// The game itself runs in a browser; this fixture exercises transport and protocol.
if (typeof process !== 'undefined' && process.versions?.node) {
  globalThis.location = {
    protocol: 'http:',
    search: '?server=' + encodeURIComponent(process.env.DYNABLASTER_TEST_SERVER || 'ws://127.0.0.1:8081/')
  };
} else {
  Module['arguments'] = new URLSearchParams(location.search).get('mode') === 'embedded' ? ['--embedded'] : [];
}

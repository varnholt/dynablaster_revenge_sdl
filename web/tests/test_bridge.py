"""Bridge integration checks: python -m unittest discover -s web/tests -v."""

import asyncio
from functools import partial
from pathlib import Path
import sys
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from bridge import relay
from websockets.asyncio.client import connect
from websockets.asyncio.server import serve
from websockets.exceptions import ConnectionClosed, InvalidStatus


class BridgeTests(unittest.IsolatedAsyncioTestCase):
    async def asyncSetUp(self):
        self.disconnected = asyncio.Event()

        async def echo(reader, writer):
            try:
                while data := await reader.read(4096):
                    writer.write(data)
                    await writer.drain()
            finally:
                writer.close()
                await writer.wait_closed()
                self.disconnected.set()

        self.tcp = await asyncio.start_server(echo, "127.0.0.1", 0)
        port = self.tcp.sockets[0].getsockname()[1]
        self.ws = await serve(partial(relay, host="127.0.0.1", port=port),
                              "127.0.0.1", 0, origins=["http://localhost:8000"],
                              subprotocols=["binary"], compression=None,
                              max_size=1024 * 1024, close_timeout=1)
        self.url = f"ws://127.0.0.1:{self.ws.sockets[0].getsockname()[1]}"

    async def asyncTearDown(self):
        self.ws.close()
        await self.ws.wait_closed()
        self.tcp.close()
        await self.tcp.wait_closed()

    def client(self):
        return connect(self.url, origin="http://localhost:8000", subprotocols=["binary"], proxy=None)

    async def test_binary_stream_and_reconnect(self):
        payload = bytes(range(256)) * 1024
        for _ in range(2):
            async with self.client() as ws:
                await ws.send(payload[:1])
                await ws.send(payload[1:19])
                await ws.send(payload[19:])
                result = bytearray()
                async with asyncio.timeout(5):
                    while len(result) < len(payload):
                        result.extend(await ws.recv())
                self.assertEqual(result, payload)
        await asyncio.wait_for(self.disconnected.wait(), 3)

    async def test_rejects_other_origins(self):
        with self.assertRaises(InvalidStatus):
            async with connect(self.url, origin="https://unrelated.example", subprotocols=["binary"], proxy=None):
                self.fail("Origin was accepted")

    async def test_rejects_text(self):
        async with self.client() as ws:
            await ws.send("not game bytes")
            with self.assertRaises(ConnectionClosed):
                await ws.recv()
            self.assertEqual(ws.close_code, 1003)
        await asyncio.wait_for(self.disconnected.wait(), 3)

    async def test_rejects_oversized_messages(self):
        async with self.client() as ws:
            await ws.send(b"x" * (1024 * 1024 + 1))
            with self.assertRaises(ConnectionClosed):
                await ws.recv()
            self.assertEqual(ws.close_code, 1009)

    async def test_unavailable_tcp_server_closes_websocket(self):
        self.tcp.close()
        await self.tcp.wait_closed()
        async with self.client() as ws:
            with self.assertRaises(ConnectionClosed):
                await ws.recv()
            self.assertEqual(ws.close_code, 1011)


if __name__ == "__main__":
    unittest.main()

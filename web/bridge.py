"""Binary WebSocket <-> fixed Dynablaster TCP endpoint. Python 3.11+."""

import argparse
import asyncio
import logging

from websockets.asyncio.server import serve
from websockets.exceptions import ConnectionClosed

LOG = logging.getLogger("dynablaster.bridge")


async def relay(websocket, host, port):
    """One TCP connection per player/bot, preserving byte order, not framing."""
    writer = None
    tasks = []
    try:
        reader, writer = await asyncio.wait_for(asyncio.open_connection(host, port), 10)

        async def to_tcp():
            async for message in websocket:
                if not isinstance(message, bytes):
                    await websocket.close(1003, "Binary game data required")
                    return
                writer.write(message)
                await writer.drain()

        async def to_websocket():
            while data := await reader.read(65536):
                await websocket.send(data)

        tasks = [asyncio.create_task(to_tcp()), asyncio.create_task(to_websocket())]
        done, _ = await asyncio.wait(tasks, return_when=asyncio.FIRST_COMPLETED)
        for task in done:
            task.result()
    except (OSError, TimeoutError, ConnectionClosed):
        LOG.info("Game connection ended", exc_info=True)
        await websocket.close(1011, "Game server unavailable or disconnected")
    finally:
        for task in tasks:
            task.cancel()
        await asyncio.gather(*tasks, return_exceptions=True)
        if writer:
            writer.close()
            try:
                await writer.wait_closed()
            except OSError:
                pass


def arguments():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--listen", default="127.0.0.1")
    parser.add_argument("--port", type=int, default=8081)
    parser.add_argument("--target-host", default="127.0.0.1")
    parser.add_argument("--target-port", type=int, default=6300)
    parser.add_argument("--origin", action="append", required=True,
                        help="Allowed page origin, e.g. http://localhost:8000; repeat for more")
    return parser.parse_args()


async def main(args):
    async def handler(websocket):
        await relay(websocket, args.target_host, args.target_port)

    async with serve(handler, args.listen, args.port, origins=args.origin,
                     subprotocols=["binary"], compression=None, max_size=1024 * 1024,
                     max_queue=8, write_limit=65536, close_timeout=2) as server:
        LOG.info("Bridge listening on %s:%s -> %s:%s", args.listen, args.port,
                 args.target_host, args.target_port)
        await server.serve_forever()


if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO)
    try:
        asyncio.run(main(arguments()))
    except KeyboardInterrupt:
        pass

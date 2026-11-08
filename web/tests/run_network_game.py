"""Run the real native server and a native or compiled WebAssembly gameplay probe."""

import argparse
import asyncio
from functools import partial
import os
from pathlib import Path
import socket
import sys
import tempfile

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from bridge import relay
from websockets.asyncio.server import serve


async def main(args):
    # Never silently attach the test to an unrelated server using the game's fixed port.
    with socket.socket() as check:
        check.bind(("127.0.0.1", 6300))
    with tempfile.TemporaryFile(mode="w+b") as log:
        server = await asyncio.create_subprocess_exec(str(Path(args.server).resolve()),
                    cwd=str(Path(__file__).resolve().parents[2] / "client"),
                    stdout=log, stderr=log)
        try:
            async with asyncio.timeout(15):
                while True:
                    if server.returncode is not None:
                        raise RuntimeError("Game server exited before listening")
                    try:
                        _, writer = await asyncio.open_connection("127.0.0.1", 6300)
                        writer.close()
                        await writer.wait_closed()
                        break
                    except OSError:
                        await asyncio.sleep(.1)
            # Node's WebSocket implementation omits Origin. Only this loopback test
            # accepts it; the production CLI requires explicit browser origins.
            async with serve(partial(relay, host="127.0.0.1", port=6300), "127.0.0.1", 0,
                             origins=[None], subprotocols=["binary"], compression=None) as bridge:
                port = bridge.sockets[0].getsockname()[1]
                env = dict(os.environ, DYNABLASTER_TEST_SERVER=f"ws://127.0.0.1:{port}/")
                command = [str(Path(args.probe).resolve())]
                if args.node:
                    command.insert(0, args.node)
                probe = await asyncio.create_subprocess_exec(*command, env=env)
                try:
                    async with asyncio.timeout(50):
                        status = await probe.wait()
                    if status:
                        raise RuntimeError(f"Network probe exited with status {status}")
                finally:
                    if probe.returncode is None:
                        probe.kill()
                        await probe.wait()
        except BaseException:
            log.seek(0)
            print(log.read().decode(errors="replace"), file=sys.stderr)
            raise
        finally:
            if server.returncode is None:
                server.terminate()
                await server.wait()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--server", required=True)
    parser.add_argument("--probe", required=True)
    parser.add_argument("--node", help="Node 22+ for a compiled .js WebAssembly probe")
    asyncio.run(main(parser.parse_args()))

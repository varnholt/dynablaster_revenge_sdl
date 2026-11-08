"""Reproducible browser game playtest; uses an isolated Chromium profile."""

import argparse
from functools import partial
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
import os
from pathlib import Path
import socket
import subprocess
import sys
import threading
import time

from playwright.sync_api import sync_playwright


def main(args):
    output = Path(args.output).resolve()
    output.mkdir(parents=True, exist_ok=True)
    handler = partial(SimpleHTTPRequestHandler, directory=str(Path(args.build).resolve()))
    server = ThreadingHTTPServer(("127.0.0.1", 0), handler)
    threading.Thread(target=server.serve_forever, daemon=True).start()
    logs = []
    errors = []
    processes = []
    process_logs = []
    try:
        if args.native_server:
            with socket.socket() as check:
                check.bind(("127.0.0.1", 6300))
            with socket.socket() as check:
                check.bind(("127.0.0.1", 0))
                bridge_port = check.getsockname()[1]
            native_log = open(output / "native-server.log", "w", encoding="utf-8")
            bridge_log = open(output / "bridge.log", "w", encoding="utf-8")
            process_logs.extend([native_log, bridge_log])
            processes.append(subprocess.Popen([str(Path(args.native_server).resolve())],
                cwd=Path(__file__).resolve().parents[2] / "client", stdout=native_log, stderr=native_log))
            processes.append(subprocess.Popen([sys.executable, str(Path(__file__).resolve().parents[1] / "bridge.py"),
                "--port", str(bridge_port), "--origin", f"http://127.0.0.1:{server.server_port}"],
                stdout=bridge_log, stderr=bridge_log))
            deadline = time.monotonic() + 15
            while True:
                try:
                    with socket.create_connection(("127.0.0.1", bridge_port), timeout=.2):
                        break
                except OSError:
                    if time.monotonic() > deadline or any(p.poll() is not None for p in processes):
                        raise AssertionError("Native server or bridge did not start")
                    time.sleep(.1)
            args.endpoint = f"ws://127.0.0.1:{bridge_port}/"
        with sync_playwright() as p:
            browser = p.chromium.launch(executable_path=os.environ.get("CHROMIUM_PATH"),
                headless=True, args=["--use-angle=swiftshader", "--enable-unsafe-swiftshader",
                                     "--autoplay-policy=no-user-gesture-required",
                                     "--disable-background-timer-throttling", "--disable-renderer-backgrounding"])
            page = browser.new_page(viewport={"width": 1280, "height": 900})
            if args.diagnose_shaders:
                page.add_init_script("""(() => {
                    const draw = WebGL2RenderingContext.prototype.drawElements;
                    let count = 0;
                    WebGL2RenderingContext.prototype.drawElements = function(...args) {
                        if (!this.getParameter(this.CURRENT_PROGRAM) && count++ < 3)
                            console.error(new Error('DRAW WITHOUT SHADER').stack);
                        return draw.apply(this, args);
                    };
                })();""")
            page.on("console", lambda message: logs.append(message.text))
            page.on("pageerror", lambda error: errors.append(str(error)))
            query = f"?server={args.endpoint}" if args.endpoint else ""
            filename = ("dynablaster_network_test.html" + (query or "?mode=embedded")) if args.probe else "dynablaster_revenge.html" + query
            page.goto(f"http://127.0.0.1:{server.server_port}/{filename}", timeout=120000)
            if args.probe:
                deadline = time.monotonic() + 65
                while time.monotonic() < deadline and not any("PASS: two clients" in line for line in logs):
                    page.wait_for_timeout(100)
                    if any("FAIL:" in line for line in logs) or errors:
                        raise AssertionError("Browser probe failed")
                assert any("PASS: two clients" in line for line in logs), "Browser probe timed out"
                kind = "WebSocket network" if args.endpoint else "embedded server"
                print(f"PASS: actual Chromium ran two clients across repeated {kind} matches")
            else:
                deadline = time.monotonic() + 120
                while time.monotonic() < deadline and not any("GL_RENDERER:" in line for line in logs):
                    page.wait_for_timeout(100)
                    if errors:
                        raise AssertionError("Browser game startup failed")
                page.wait_for_timeout(3500)
                canvas = page.locator("#canvas")
                canvas.screenshot(path=str(output / "menu.png"))
                if args.steps:
                    for step in Path(args.steps).read_text().splitlines():
                        words = step.split()
                        if not words or words[0].startswith("#"):
                            continue
                        if words[0] == "wait":
                            page.wait_for_timeout(int(words[1]))
                        elif words[0] == "wait-log":
                            expected = " ".join(words[1:])
                            deadline = time.monotonic() + 90
                            while not any(expected in line for line in logs):
                                assert time.monotonic() < deadline, f"Timed out waiting for gameplay evidence: {expected}"
                                assert not errors, f"Browser runtime errors: {errors}"
                                page.wait_for_timeout(100)
                        elif words[0] == "click":
                            box = canvas.bounding_box()
                            canvas.click(position={"x": float(words[1]) * box["width"] / 1920,
                                                   "y": float(words[2]) * box["height"] / 1080})
                        elif words[0] == "key":
                            page.keyboard.down(words[1])
                            page.wait_for_timeout(int(words[2]))
                            page.keyboard.up(words[1])
                        elif words[0] == "shot":
                            canvas.screenshot(path=str(output / words[1]))
                for expected in args.expect:
                    assert any(expected in line for line in logs), f"Missing gameplay evidence: {expected}"
                assert not errors, f"Browser runtime errors: {errors}"
                invalid_gl = [line for line in logs if "WebGL: INVALID_" in line or "shader compile error" in line or "program link error" in line]
                assert not invalid_gl, f"Browser rendering errors: {invalid_gl[:3]}"
                print("PASS: full browser game rendered and completed the requested playtest steps without runtime or WebGL errors")
            browser.close()
    finally:
        (output / "console.log").write_text("\n".join(logs), encoding="utf-8")
        (output / "errors.log").write_text("\n".join(errors), encoding="utf-8")
        server.shutdown()
        server.server_close()
        for process in processes:
            if process.poll() is None:
                process.terminate()
            process.wait(timeout=10)
        for log in process_logs:
            log.close()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--endpoint")
    parser.add_argument("--native-server", help="Start an owned native server and bridge for the rendered network playtest")
    parser.add_argument("--steps")
    parser.add_argument("--probe", action="store_true")
    parser.add_argument("--expect", action="append", default=[])
    parser.add_argument("--diagnose-shaders", action="store_true")
    main(parser.parse_args())

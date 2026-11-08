"""Run the regular Windows game through its built-in development script hooks."""

import argparse
import os
from pathlib import Path
import subprocess


def main(args):
    executable = Path(args.executable).resolve()
    output = Path(args.output).resolve()
    output.mkdir(parents=True, exist_ok=True)
    commands = ["wait 3000"]
    expected_logs = []
    for line in Path(args.steps).read_text().splitlines():
        words = line.split()
        if not words or words[0].startswith("#"):
            continue
        if words[0] == "wait-log":
            expected_logs.append(" ".join(words[1:]))
            continue
        if words[0] == "shot":
            words[1] = (output / words[1]).as_posix()
        if words[0] == "key":
            words[1] = words[1].removeprefix("Arrow")
        commands.append(" ".join(words))
    commands.append("quit")
    script = output / "playtest.txt"
    script.write_text("\n".join(commands), encoding="utf-8")
    with open(output / "game.log", "w", encoding="utf-8") as log:
        result = subprocess.run([str(executable)], cwd=executable.parent,
            env=dict(os.environ, DYNABLASTER_DEV_SCRIPT=str(script)),
            stdout=log, stderr=log, timeout=120)
    assert result.returncode == 0, f"Game exited with status {result.returncode}"
    logs = (output / "game.log").read_text(encoding="utf-8")
    assert "game 1 started" in logs, "The regular Windows game never started gameplay"
    assert all(expected in logs for expected in expected_logs), "Missing scripted gameplay evidence"
    assert all((output / name).is_file() for name in
               ["lobby.png", "gameplay-before.png", "bomb.png", "gameplay-after.png", "menu-after.png"])
    assert "server initialized" in logs, "The regular local server did not listen"
    assert logs.count("login request packet received") >= 3, "Player and two bots did not connect"
    print("PASS: regular Windows game hosted locally, connected two bots, started, accepted scripted controls and exited cleanly")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--executable", required=True)
    parser.add_argument("--steps", required=True)
    parser.add_argument("--output", required=True)
    main(parser.parse_args())

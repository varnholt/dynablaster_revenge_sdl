# Builds the story mode's intro into client/data/game/story/intro from scratch: python build_all.py
#   import_hjb.py      the player from the game's player-*.hjb, into Blender
#   build_player.py    its intro actions (panic, ride, look_up) and the black bomberman
#   build_dragon.py    the dragon
#   build_princess.py  the princess
#   build_doctor.py    the doctor
#   build_lab.py       the lab from the first world's scenery with its door open and its lit windows, its dome
#                      in the plates and pieces that burst (lab.json: where it all is, for build_intro.py)
#   build_backdrop.py  the mountains with the castle
#   build_room.py      the ending's machine room in the castle
#   build_tears.py     the black bomberman's tears in the ending
#   backdrop_detail.py their rock up close (plain python, after Blender)
#   room_detail.py     the room's bricks and blocks up close (plain python, after Blender)
#   build_sky.py       the night sky's map (python, no blender)
#   build_intro.py     stages the intro: intro.txt, the timeline StoryCutsceneDrawable plays
#   build_ending.py    stages the ending: ending.txt
# Blender is found via the BLENDER environment variable or in its default install folder.
import glob
import os
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "out")
INTRO = os.path.join(HERE, "..", "..", "client", "data", "game", "story", "intro")
FILES = ["lab.glb", "door.glb", "lights.glb", "backdrop.glb", "dome.glb", "dragon.glb", "princess.glb", "doctor.glb",
         "player.glb", "player_dark.glb", "sky.tga",
         "intro.txt", "room.glb", "tears.glb", "ending.txt"]


def blender():
    if os.environ.get("BLENDER"):
        return os.environ["BLENDER"]
    found = sorted(glob.glob(r"C:\Program Files\Blender Foundation\Blender *\blender.exe"))
    return found[-1] if found else "blender"


def run_blender(script, *args, blend=None):
    command = [blender(), "--background"]
    command += [blend] if blend else ["--factory-startup"]
    command += ["--python", os.path.join(HERE, script), "--", *args]
    result = subprocess.run(command, capture_output=True, text=True)
    lines = [line for line in result.stdout.splitlines() if line.startswith(("RESULT", "Error"))]
    print("\n".join(lines))
    failed = any(line.startswith("Error") for line in lines) or "Traceback" in result.stderr + result.stdout
    if result.returncode or failed:
        sys.exit(f"{script} failed:\n{result.stdout[-3000:]}\n{result.stderr[-3000:]}")


os.makedirs(OUT, exist_ok=True)
os.makedirs(INTRO, exist_ok=True)
run_blender("import_hjb.py", os.path.join(OUT, "player.blend"), os.path.join(OUT, "player.glb"))
run_blender("build_player.py", OUT, blend=os.path.join(OUT, "player.blend"))
subprocess.run([sys.executable, os.path.join(HERE, "build_sky.py"), OUT], check=True)
for script in ("build_dragon.py", "build_princess.py", "build_doctor.py", "build_lab.py", "build_backdrop.py",
               "build_room.py", "build_tears.py", "build_intro.py", "build_ending.py"):
    run_blender(script, OUT)
subprocess.run([sys.executable, os.path.join(HERE, "backdrop_detail.py"), OUT], check=True)
subprocess.run([sys.executable, os.path.join(HERE, "room_detail.py"), OUT], check=True)
for name in FILES:
    shutil.copy(os.path.join(OUT, name), INTRO)
print("intro data in", os.path.normpath(INTRO))

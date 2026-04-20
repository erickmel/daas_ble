from pathlib import Path
import shutil

Import("env")

def copy_firmware(target, source, env):
    src = Path(str(target[0]))  # the built .bin file
    dest_dir = Path(r"./versions")
    dest_dir.mkdir(parents=True, exist_ok=True)

    dest_file = dest_dir / src.name
    shutil.copy2(src, dest_file)

    print(f"Copied: {src} -> {dest_file}")

env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", copy_firmware)
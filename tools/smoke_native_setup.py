"""Open the extracted setup launcher; retain CI screenshot as a UI proof."""
import os
import pathlib
import platform
import subprocess
import time
import zipfile
import sys

folder = pathlib.Path('build-native-smoke').resolve()
folder.mkdir(exist_ok=True)
with zipfile.ZipFile(sys.argv[1]) as z:
    z.extractall(folder)
    for info in z.infolist():
        if not info.is_dir():
            (folder / info.filename).chmod((info.external_attr >> 16) & 0o777 or 0o644)
proc = subprocess.Popen([str(folder / 'Wipeout2K97XL')], cwd=folder)
try:
    time.sleep(12)
    assert proc.poll() is None, f'Launcher exited with {proc.returncode}'
    if platform.system() == 'Linux':
        windows = subprocess.check_output(['xdotool', 'search', '--onlyvisible', '--name', 'Wipeout2K97XL'], text=True).split()
        assert windows, 'No visible launcher window'
        subprocess.run(['import', '-window', windows[0], 'native-setup.png'], check=True)
    else:
        subprocess.run(['screencapture', '-x', 'native-setup.png'], check=True)
    assert pathlib.Path('native-setup.png').stat().st_size > 1000
    print('Extracted setup launcher remains running; screenshot captured.')
finally:
    proc.terminate()
    try:
        proc.wait(timeout=10)
    except subprocess.TimeoutExpired:
        proc.kill()

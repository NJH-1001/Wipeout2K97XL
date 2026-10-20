"""Compare the released AppImage with its exact unwrapped Linux executable."""
import hashlib
import json
import os
from pathlib import Path
import signal
import subprocess
import time
import urllib.request

root = Path(__file__).resolve().parents[1]
work = root / 'build-appimage-host-diagnostic'
work.mkdir(exist_ok=True)
candidate=os.environ.get('WXL_DIAGNOSTIC_APPIMAGE')
if candidate:
    image=Path(candidate).resolve()
    proof=json.loads((image.parent/'appimage-build-proof.json').read_text())
else:
    proof=json.loads((root/'docs/verification/appimage.json').read_text())
    image=work/proof['artifact']
    url='https://github.com/NJH-1001/Wipeout2K97XL/releases/download/v0.1.1/'+image.name
    with urllib.request.urlopen(url,timeout=120) as response:
        image.write_bytes(response.read())
assert hashlib.sha256(image.read_bytes()).hexdigest()==proof['sha256']
image.chmod(0o755)
subprocess.run([str(image), '--appimage-extract'], cwd=work, stdout=subprocess.DEVNULL, check=True)
kit = work / 'squashfs-root/usr/share/wipeoutxl/kit'
exe = kit / 'Wipeout2K97XL'
exe.chmod(0o755)
results = {}
for kind, command, cwd, extra in [
    ('linux-kit', [str(exe), '--launcher'], kit, {}),
    ('appimage', [str(image), '--appimage-extract-and-run', '--launcher'], work,
     {'WXL_APPIMAGE_DATA_DIR': str(work / 'profile')})
]:
    env = {**os.environ, **extra}
    proc = subprocess.Popen(command, cwd=cwd, env=env, stdout=subprocess.PIPE,
                            stderr=subprocess.PIPE, start_new_session=True)
    time.sleep(12)
    windows = subprocess.run(['xdotool', 'search', '--onlyvisible', '--name', 'Wipeout2K97XL'],
                             capture_output=True, text=True).stdout.split()
    alive = proc.poll() is None
    if windows:
        subprocess.run(['import', '-window', windows[0], str(work / (kind + '.png'))], check=True)
    if alive:
        os.killpg(proc.pid, signal.SIGTERM)
    try:
        stdout, stderr = proc.communicate(timeout=10)
    except subprocess.TimeoutExpired:
        os.killpg(proc.pid, signal.SIGKILL)
        stdout, stderr = proc.communicate(timeout=10)
    results[kind] = {'alive_before_cleanup': alive, 'visible_window': bool(windows),
                     'stdout': stdout.decode(errors='replace')[-16000:],
                     'stderr': stderr.decode(errors='replace')[-16000:]}
    time.sleep(1)
(work / 'result.json').write_text(json.dumps(results, indent=2))
print(json.dumps(results, indent=2))
assert results['linux-kit']['alive_before_cleanup'] and results['linux-kit']['visible_window'], 'Baseline Linux kit did not open'

if candidate:
    assert results["appimage"]["alive_before_cleanup"] and results["appimage"]["visible_window"], "AppImage did not open on newer host"

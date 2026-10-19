"""Audit AppImage contents/ELFs and exercise the real writable setup launcher."""
import hashlib
import json
import os
from pathlib import Path
import re
import signal
import subprocess
import sys
import tempfile
import time
import zipfile

image = Path(sys.argv[1]).resolve()
root = Path(__file__).resolve().parents[1]
assert image.read_bytes()[:4] == b'\x7fELF' and image.read_bytes()[8:11] == b'AI\x02'
with tempfile.TemporaryDirectory(prefix='wxl-appimage-audit-') as tmp:
    tmp = Path(tmp)
    subprocess.run([str(image), '--appimage-extract'], cwd=tmp, stdout=subprocess.DEVNULL, check=True)
    appdir = tmp / 'squashfs-root'
    kit = appdir / 'usr/share/wipeoutxl/kit'
    original = root / 'build-appimage/linux_kit'
    with zipfile.ZipFile(original) as z:
        for entry in z.infolist():
            if not entry.is_dir():
                assert (kit / entry.filename).read_bytes() == z.read(entry), entry.filename
    allowed = {'psxrecomp/bios/openbios.bin', 'psxrecomp/beetle-psx/deps/openbios/openbios.bin'}
    for p in kit.rglob('*'):
        if not p.is_file():
            continue
        rel = p.relative_to(kit)
        assert rel.parts[0] not in {'disc', 'generated', 'prepared_disc', 'analysis', 'captures', 'saves'}
        assert '.git' not in rel.parts
        assert p.suffix.lower() not in {'.bin', '.cue', '.iso', '.chd', '.mcr', '.mcd', '.gpr', '.bmp'} or rel.as_posix() in allowed, rel
        assert p.name not in {'settings.toml', 'state.toml', 'disc.cfg', 'bios.cfg', 'psx_bios_disasm.txt'}, rel
    env = {**os.environ, 'LD_LIBRARY_PATH': str(appdir / 'usr/lib') + ':' + str(appdir / 'usr/python/lib')}
    count = 0
    runtime_files = [kit / 'Wipeout2K97XL',
                     kit / 'psxrecomp/recompiler/build/psxrecomp-game',
                     kit / 'psxrecomp/recompiler/build/psxrecomp-bios']
    runtime_files += list((appdir / 'usr/python').rglob('*')) + list((appdir / 'usr/lib').rglob('*'))
    for p in runtime_files:
        if not p.is_file():
            continue
        with p.open('rb') as stream:
            header = stream.read(20)
        if header[:4] != b'\x7fELF' or int.from_bytes(header[16:18], 'little') not in (2, 3):
            continue
        assert header[4] == 2 and int.from_bytes(header[18:20], 'little') == 62, p
        result = subprocess.run(['ldd', str(p)], env=env, capture_output=True, text=True)
        assert 'not found' not in result.stdout, (p, result.stdout)
        for line in result.stdout.splitlines():
            if '=>' not in line:
                continue
            name, value = line.split('=>', 1)
            location = value.strip().split()[0]
            if location.startswith(str(appdir)):
                continue
            assert re.fullmatch(r'lib(c|m|dl|pthread|rt|resolv|util|OpenGL|GLdispatch)\.so\.[0-9]+', name.strip()), (p, line)
        count += 1
    subprocess.run([str(appdir / 'usr/python/bin/python3'), '-I', '-c', 'import ssl,ctypes,tomllib,urllib.request; urllib.request.urlopen("https://www.python.org/robots.txt",timeout=30).close()'],
                   env={**env, 'SSL_CERT_FILE': str(appdir / 'usr/python/ssl/cert.pem')}, check=True)

    # Run the actual single-file artifact, including extraction (no FUSE required).
    home = tmp / 'writable data'
    env = {**os.environ, 'WXL_APPIMAGE_DATA_DIR': str(home), 'APPIMAGE_EXTRACT_AND_RUN': '1'}
    proc = subprocess.Popen([str(image)], env=env, cwd=tmp, start_new_session=True)
    try:
        time.sleep(14)
        assert proc.poll() is None, proc.returncode
        windows = subprocess.check_output(['xdotool', 'search', '--onlyvisible', '--name', 'Wipeout2K97XL'], text=True).split()
        assert windows
        subprocess.run(['import', '-window', windows[0], str(root / 'appimage-launcher.png')], check=True)
        installs = list((home / 'versions').glob('*/.appimage-ready'))
        assert len(installs) == 1
        install = installs[0].parent
        assert (install / 'saves').resolve() == (home / 'profile/saves').resolve()
        assert (install / '.appimage-python/bin/python3').exists()
    finally:
        os.killpg(proc.pid, signal.SIGTERM)
        try:
            proc.wait(timeout=10)
        except subprocess.TimeoutExpired:
            os.killpg(proc.pid, signal.SIGKILL)
        # The AppImage extraction supervisor can exit before its AppRun/player.
        # Wait for all non-zombie group members before deleting their data tree.
        def active_group():
            active = []
            for stat in Path('/proc').glob('[0-9]*/stat'):
                try:
                    fields = stat.read_text().rsplit(')', 1)[1].split()
                    if int(fields[2]) == proc.pid and fields[0] != 'Z':
                        active.append(stat.parent.name)
                except (OSError, ValueError, IndexError):
                    pass
            return active
        deadline = time.monotonic() + 10
        while active_group() and time.monotonic() < deadline:
            time.sleep(0.2)
        if active_group():
            os.killpg(proc.pid, signal.SIGKILL)
            time.sleep(0.5)
        assert not active_group(), 'AppImage child processes did not stop'
proof = json.loads((root / 'dist/appimage-build-proof.json').read_text())
proof.update({'content_audit': 'passed', 'elf_architecture_and_dependencies': 'passed',
              'elf_files_checked': count, 'portable_python_https': 'passed',
              'single_file_launcher_smoke': 'passed', 'generated_game_and_steam_hardware': 'unverified'})
(root / 'dist/appimage-build-proof.json').write_text(json.dumps(proof, indent=2))
print(json.dumps(proof, indent=2))

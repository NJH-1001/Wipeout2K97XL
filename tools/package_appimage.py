"""Reproducible inputs for a writable, owner-generated Steam Deck setup AppImage."""
import copy
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import tarfile
import urllib.request
import zipfile

root = Path(__file__).resolve().parents[1]
work = root / 'build-appimage'
work.mkdir(exist_ok=True)
pins = json.loads((root / 'packaging/appimage/dependencies.json').read_text())
inputs = {}
for name, pin in pins.items():
    dest = work / name
    if not dest.exists() or hashlib.sha256(dest.read_bytes()).hexdigest() != pin['sha256']:
        request = urllib.request.Request(pin['url'], headers={'User-Agent': 'Wipeout2K97XL-packager'})
        with urllib.request.urlopen(request, timeout=120) as response, dest.open('wb') as out:
            shutil.copyfileobj(response, out)
    assert hashlib.sha256(dest.read_bytes()).hexdigest() == pin['sha256'], name
    inputs[name] = dest

appdir = work / 'Wipeout2K97XL.AppDir'
assert not appdir.exists(), 'Use a fresh build-appimage directory'
kit = appdir / 'usr/share/wipeoutxl/kit'
kit.mkdir(parents=True)
with zipfile.ZipFile(inputs['linux_kit']) as z:
    assert z.testzip() is None
    z.extractall(kit)
    for info in z.infolist():
        if not info.is_dir():
            (kit / info.filename).chmod((info.external_attr >> 16) & 0o777 or 0o644)

# This preserves the previously verified ELF binaries exactly.
shutil.copy2(root / 'docs/steam-deck.md', kit / 'docs/steam-deck.md')
with tarfile.open(inputs['python'], 'r:gz') as t:
    t.extractall(appdir / 'usr', filter='data')
python_root = appdir / 'usr/python'
(python_root / 'ssl').mkdir(exist_ok=True)
shutil.copy2('/etc/ssl/certs/ca-certificates.crt', python_root / 'ssl/cert.pem')
lib = python_root / 'lib/appimage'
lib.mkdir()
needed = {'libz.so.1'}
ldcache = subprocess.check_output(['ldconfig', '-p'], text=True)
for name in needed:
    matches = [line.split('=>', 1)[1].strip() for line in ldcache.splitlines()
               if line.strip().startswith(name + ' ') and 'x86-64' in line]
    assert matches, name
    shutil.copy2(matches[0], lib / name, follow_symlinks=True)

licenses = kit / 'docs/licenses/appimage'
licenses.mkdir(parents=True)
for package in ['zlib1g', 'ca-certificates']:
    shutil.copy2('/usr/share/doc/' + package + '/copyright', licenses / (package + '-copyright.txt'))
# Resolve optional CPython extension dependencies too. Keep glibc and vendor-neutral
# OpenGL on the host; never bundle a GPU driver or the dynamic loader.
core = re.compile(r'lib(c|m|dl|pthread|rt|resolv|util|OpenGL|GLdispatch)\.so\.[0-9]+')
env = {**os.environ, 'LD_LIBRARY_PATH': str(lib) + ':' + str(python_root / 'lib')}
runtime_files = list(python_root.rglob('*'))
for p in runtime_files:
    if not p.is_file():
        continue
    with p.open('rb') as stream:
        header = stream.read(20)
    if header[:4] != b'\x7fELF' or int.from_bytes(header[16:18], 'little') not in (2, 3):
        continue
    deps = subprocess.run(['ldd', str(p)], env=env, capture_output=True, text=True)
    assert 'not found' not in deps.stdout, (p, deps.stdout)
    for line in deps.stdout.splitlines():
        if '=>' not in line:
            continue
        name, value = line.split('=>', 1)
        name = name.strip()
        location = value.strip().split()[0]
        if core.fullmatch(name) or location.startswith(str(appdir)):
            continue
        source = Path(location)
        assert source.is_file(), line
        shutil.copy2(source, lib / name, follow_symlinks=True)
        ownership = subprocess.run(['dpkg-query', '-S', str(source)], capture_output=True, text=True)
        if ownership.returncode:
            alternative = str(source).replace('/lib/', '/usr/lib/', 1) if str(source).startswith('/lib/') else str(source.resolve())
            ownership = subprocess.run(['dpkg-query', '-S', alternative], capture_output=True, text=True)
        assert ownership.returncode == 0, (source, ownership.stderr)
        package = ownership.stdout.split(': ', 1)[0].split(':', 1)[0]
        shutil.copy2('/usr/share/doc/' + package + '/copyright', licenses / (package + '-copyright.txt'))
# Keep private dependency lookup within Python's copied tree. Never export its
# LD_LIBRARY_PATH to the launcher, Mesa, emitters, compiler or their children.
for p in python_root.rglob('*'):
    if not p.is_file() or p.is_symlink():
        continue
    with p.open('rb') as stream:
        header=stream.read(20)
    if header[:4] != b'\x7fELF' or int.from_bytes(header[16:18],'little') not in (2,3):
        continue
    existing=subprocess.check_output(['patchelf','--print-rpath',str(p)],text=True).strip()
    private='$ORIGIN/' + os.path.relpath(lib,p.parent)
    runpath=':'.join(dict.fromkeys([x for x in existing.split(':') if x]+[private]))
    subprocess.run(['patchelf','--set-rpath',runpath,str(p)],check=True)
for name, url in {
    'type2-runtime-LICENSE.txt': 'https://raw.githubusercontent.com/AppImage/type2-runtime/main/LICENSE',
    'appimagetool-LICENSE.txt': 'https://raw.githubusercontent.com/AppImage/appimagetool/main/LICENSE',
}.items():
    # Distribution notices; binaries above are independently checksum-pinned.
    with urllib.request.urlopen(url, timeout=30) as r:
        (licenses / name).write_bytes(r.read())
for source, target in [('packaging/appimage/AppRun', 'AppRun'),
                       ('packaging/appimage/Wipeout2K97XL.desktop', 'Wipeout2K97XL.desktop'),
                       ('assets/psxrecomp.png', 'wipeoutxl.png')]:
    shutil.copy2(root / source, appdir / target)
(appdir / 'AppRun').chmod(0o755)
# Versioned install identity changes when any distributed content changes.
h = hashlib.sha256()
for p in sorted(appdir.rglob('*')):
    if p.is_file():
        h.update(str(p.relative_to(appdir)).encode() + b'\0')
        h.update(p.read_bytes())
(appdir / 'payload-id').write_text(h.hexdigest() + '\n')
outdir = root / 'dist'
outdir.mkdir(exist_ok=True)
revision=(root/'packaging/appimage/REVISION').read_text().strip()
assert re.fullmatch(r'r[0-9]+',revision)
output = outdir / ('wipeoutxl-' + (root / 'VERSION').read_text().strip() + '-steamdeck-x86_64-' + revision + '.AppImage')
inputs['appimagetool'].chmod(0o755)
env = {**os.environ, 'ARCH': 'x86_64', 'APPIMAGE_EXTRACT_AND_RUN': '1'}
subprocess.run([str(inputs['appimagetool']), '--runtime-file', str(inputs['runtime']),
                str(appdir), str(output)], env=env, check=True)
output.chmod(0o755)
digest = hashlib.sha256(output.read_bytes()).hexdigest()
output.with_suffix('.AppImage.sha256').write_text(digest + '  ' + output.name + '\n')
(outdir / 'appimage-build-proof.json').write_text(json.dumps({
    'artifact': output.name, 'sha256': digest, 'inputs': pins,
    'payload_id': h.hexdigest(), 'runtime_binaries': 'unchanged from pinned released Linux ZIP',
    'physical_steam_deck_and_2026_controller': 'unverified',
    'dependency_scope': 'host libraries for game/build tools; private RUNPATH only for bundled Python',
    'packaging_revision': revision,
}, indent=2))
print(output, digest)

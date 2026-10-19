"""Check real bootstrap persistence, update isolation and Steam-input environment."""
import json
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='wxl launcher ') as temp:
    tmp = Path(temp)
    appdir = tmp / 'read only AppDir'
    kit = appdir / 'usr/share/wipeoutxl/kit'
    kit.mkdir(parents=True)
    for name in ['input.ini', 'keybinds.ini']:
        (kit / name).write_text('default binding\n')
    exe = kit / 'Wipeout2K97XL'
    exe.write_text('#!/bin/sh\nprintf "%s\\n" "$PWD" "$SDL_JOYSTICK_HIDAPI_STEAMDECK" "$SDL_GAMECONTROLLER_ALLOW_STEAM_VIRTUAL_GAMEPAD" "$RETCOMM_PYTHON" "$@"\n')
    exe.chmod(0o755)
    (appdir / 'usr/lib').mkdir()
    (appdir / 'usr/python/bin').mkdir(parents=True)
    (appdir / 'usr/python/bin/python3').write_text('placeholder')
    (appdir / 'payload-id').write_text('a' * 64)
    run = appdir / 'AppRun'
    shutil.copy2(root / 'packaging/appimage/AppRun', run)
    run.chmod(0o755)
    env = {**os.environ, 'APPDIR': str(appdir), 'WXL_APPIMAGE_DATA_DIR': str(tmp / 'persistent data')}
    for key in ['SDL_JOYSTICK_HIDAPI_STEAMDECK', 'SDL_GAMECONTROLLER_ALLOW_STEAM_VIRTUAL_GAMEPAD',
                'WXL_STEAM_INPUT_MODE', 'RETCOMM_PYTHON']:
        env.pop(key, None)
    def launch(extra=None):
        return subprocess.check_output([str(run), 'argument with spaces'], env={**env, **(extra or {})}, text=True).splitlines()
    first = launch()
    install = Path(first[0])
    assert first[1:3] == ['0', '1'] and first[-1] == 'argument with spaces', first
    assert Path(first[3]).is_relative_to(install)
    assert install.is_relative_to(tmp / 'persistent data')
    (install / 'input.ini').write_text('custom player binding\n')
    (install / 'saves/test.mcr').write_text('synthetic save')
    assert (install / 'build-release/saves/test.mcr').read_text() == 'synthetic save'
    (install / 'build-release/generated-local.c').write_text('synthetic locally generated file')
    assert launch() == first
    assert (install / 'input.ini').read_text() == 'custom player binding\n'
    assert (install / 'build-release/generated-local.c').is_file()
    (appdir / 'payload-id').write_text('b' * 64)
    second = launch()
    assert second[0] != first[0]
    assert (Path(second[0]) / 'input.ini').read_text() == 'custom player binding\n'
    assert (Path(second[0]) / 'saves/test.mcr').read_text() == 'synthetic save'
    assert launch({'WXL_STEAM_INPUT_MODE': 'direct-deck'})[1] == '1'
    assert launch({'SDL_JOYSTICK_HIDAPI_STEAMDECK': '1'})[1] == '1'
    assert subprocess.run([str(run)], env={**env, 'WXL_STEAM_INPUT_MODE': 'invalid'}, capture_output=True).returncode == 2
    (appdir / 'payload-id').write_text('../escape')
    assert subprocess.run([str(run)], env=env, capture_output=True).returncode != 0
print('PASS: writable bootstrap, spaces, repeat launch, shared saves/settings, update isolation and Steam input policies')

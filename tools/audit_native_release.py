"""Native setup archive content, architecture and dependency release gate."""
import hashlib
import json
import pathlib
import platform
import subprocess
import sys
import tempfile
import zipfile

archive = pathlib.Path(sys.argv[1])
target = sys.argv[2]
executables = ['Wipeout2K97XL', 'psxrecomp/recompiler/build/psxrecomp-game',
               'psxrecomp/recompiler/build/psxrecomp-bios']
with zipfile.ZipFile(archive) as z:
    assert z.testzip() is None
    names = set(z.namelist())
    for required in executables + ['game.toml', 'input.ini', 'src/negcon_mod.c',
                                  'mods/bundled/wxl.controls.negcon/1.0.0/manifest.toml']:
        assert required in names, required
    allowed_roms = {'psxrecomp/bios/openbios.bin',
                   'psxrecomp/beetle-psx/deps/openbios/openbios.bin'}
    for name in names:
        p = pathlib.PurePosixPath(name)
        assert not p.is_absolute() and '..' not in p.parts, name
        assert p.parts[0] not in {'disc', 'generated', 'prepared_disc', 'analysis', 'captures', 'saves'}, name
        assert '.git' not in p.parts, name
        assert p.suffix.lower() not in {'.bin', '.cue', '.iso', '.chd', '.mcd', '.mcr', '.gpr', '.bmp', '.exe', '.dll'} or name in allowed_roms, name
        assert p.name not in {'state.toml', 'settings.toml', 'disc.cfg', 'bios.cfg', 'psx_bios_disasm.txt'}, name
        assert not p.name.startswith(('psx_freeze_', 'psx_last_run_', 'psx_crash')), name
    with tempfile.TemporaryDirectory() as tmp:
        for name in executables:
            p = pathlib.Path(z.extract(name, tmp))
            assert (z.getinfo(name).external_attr >> 16) & 0o111, name
            p.chmod(0o755)
            description = subprocess.check_output(['file', str(p)], text=True).strip()
            if target == 'linux-x64':
                assert 'ELF 64-bit' in description and 'x86-64' in description, description
                deps = subprocess.check_output(['ldd', str(p)], text=True)
                assert 'not found' not in deps, deps
                assert 'libSDL' not in deps, 'SDL must be statically linked'
            else:
                assert 'Mach-O 64-bit' in description, description
                assert ('arm64' if target.endswith('arm64') else 'x86_64') in description, description
                deps = subprocess.check_output(['otool', '-L', str(p)], text=True)
                for dep in deps.splitlines()[1:]:
                    assert dep.strip().startswith(('/usr/lib/', '/System/Library/')), dep
            print(description)
            print(deps)
            if name != executables[0]:
                result = subprocess.run([str(p), '--help'], capture_output=True, timeout=30)
                assert result.returncode in (0, 1), (name, result.returncode)
digest = hashlib.sha256(archive.read_bytes()).hexdigest()
archive.with_suffix('.zip.sha256').write_text(digest + '  ' + archive.name + '\n')
print(json.dumps({'target': target, 'sha256': digest, 'entries': len(names),
                  'native_runner': platform.platform(), 'content_and_dependencies': 'passed'}, indent=2))

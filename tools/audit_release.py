"""Fail closed on private/generated content in a public setup ZIP."""
import hashlib,json,pathlib,sys,zipfile,subprocess,tempfile,re,shutil
p=pathlib.Path(sys.argv[1])
with zipfile.ZipFile(p) as z:
 assert z.testzip() is None, "ZIP CRC failure"
 bad=[]
 roms={"psxrecomp/bios/openbios.bin", "psxrecomp/beetle-psx/deps/openbios/openbios.bin"}
 required={"Wipeout2K97XL.exe","game.toml","input.ini","keybinds.ini","gamecontrollerdb.txt",
 "src/negcon_mod.c","src/negcon_controls.h","psxrecomp/recompiler/build/psxrecomp-game.exe",
 "psxrecomp/recompiler/build/psxrecomp-bios.exe","mods/bundled/wxl.controls.negcon/1.0.0/manifest.toml"}
 names=set(z.namelist()); assert required<=names, sorted(required-names)
 for n in names:
  q=pathlib.PurePosixPath(n); low=q.name.lower()
  if (q.parts and q.parts[0] in {"disc","generated","prepared_disc","analysis","captures","saves"}) or ".git" in q.parts:
   bad.append(n)
  if q.suffix.lower() in {".bin",".cue",".iso",".chd",".mcd",".mcr",".gpr",".bmp"} and n not in roms: bad.append(n)
  if low.startswith(("psx_freeze_","psx_last_run_","psx_crash")) or low in {"state.toml","settings.toml","disc.cfg","bios.cfg","psx_bios_disasm.txt"}: bad.append(n)
 assert not bad, sorted(set(bad))
 data=z.read("Wipeout2K97XL.exe"); pe=int.from_bytes(data[60:64],"little")
 assert data[pe:pe+4]==b"PE\0\0" and int.from_bytes(data[pe+4:pe+6],"little")==0x8664
 # Inspect explicit PE filenames; do not trust MSYS extensionless aliases.
 objdump=shutil.which("x86_64-w64-mingw32-objdump") or shutil.which("objdump")
 assert objdump, "objdump is required for the dependency audit"
 system=re.compile(r"^(KERNEL32|KERNELBASE|USER32|GDI32|GDIPLUS|ADVAPI32|SHELL32|SHCORE|OLE32|OLEAUT32|WS2_32|WINMM|IMM32|SETUPAPI|VERSION|OPENGL32|GLU32|D2D1|DWRITE|DCOMP|DXGI|D3D[0-9]*|D3DCOMPILER_[0-9]*|WINDOWSCODECS|PROPSYS|COMCTL32|COMDLG32|RPCRT4|SHLWAPI|CRYPT32|BCRYPT|NCRYPT|IPHLPAPI|NSI|DNSAPI|MSVCRT|UCRTBASE|VCRUNTIME[0-9]*|MSVCP[0-9]*|DBGHELP|DWMAPI|UXTHEME|POWRPROF|CFGMGR32|HID|WINTRUST|MSIMG32|AVRT|MF[A-Z]*|AUDIOSES|DINPUT8|XINPUT[0-9_]*|USERENV|API-MS-.*|EXT-MS-.*)\.DLL$",re.I)
 pes=sorted(n for n in names if pathlib.PurePosixPath(n).suffix.lower() in {".exe",".dll"})
 missing=[]
 with tempfile.TemporaryDirectory(prefix="wxl-pe-audit-") as tmp:
  for n in pes:
   z.extract(n,tmp)
   r=subprocess.run([objdump,"-p",str(pathlib.Path(tmp)/n)],capture_output=True,text=True,check=True)
   for dll in re.findall(r"DLL Name:\s*(\S+)",r.stdout):
    if system.fullmatch(dll):continue
    expected=(pathlib.PurePosixPath(n).parent/dll).as_posix().lower()
    if expected not in {x.lower() for x in names}:missing.append((n,dll))
 assert not missing, missing
 result={"sha256":hashlib.sha256(p.read_bytes()).hexdigest(),"bytes":p.stat().st_size,"entries":len(names),"forbidden_entries":[],"crc":"passed","architecture":"AMD64","dependency_audit":"passed","pe_files_checked":len(pes)}
 p.with_suffix(".zip.sha256").write_text(result["sha256"]+"  "+p.name+"\n",encoding="utf-8")
 print(json.dumps(result,indent=2))

"""Fail closed on private/generated content in a public setup ZIP."""
import hashlib,json,pathlib,sys,zipfile
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
 result={"sha256":hashlib.sha256(p.read_bytes()).hexdigest(),"bytes":p.stat().st_size,"entries":len(names),"forbidden_entries":[],"crc":"passed","architecture":"AMD64"}
 p.with_suffix(".zip.sha256").write_text(result["sha256"]+"  "+p.name+"\n",encoding="utf-8")
 print(json.dumps(result,indent=2))

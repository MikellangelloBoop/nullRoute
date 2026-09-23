"""Record a verified MetaHuman package and retain evidence from previous builds."""
import hashlib
import json
import re
import shutil
import sys
from datetime import datetime
from pathlib import Path

root = Path(__file__).resolve().parent.parent
release = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else root / "Releases/Windows"
assert release.is_relative_to(root / "Releases"), release
folder = release / "MetaHuman"
folder.mkdir(exist_ok=True)
sha = lambda path: hashlib.sha256(path.read_bytes()).hexdigest()
read = lambda path: path.read_text(encoding="utf-8-sig", errors="replace")
cases = {}
for case in ["Breacher", "Module", "Stance"]:
    log = read(root / f"Saved/PackagedBreacher-{case}.log")
    assert f"NR_{case.upper()}_COMPLETE PASSED" in log, case
    assert not re.search(rf"NR_{case.upper()} .+ : FAIL", log), case
    cases[case] = len(re.findall(rf"LogTemp: Display: NR_{case.upper()} .+ : PASS", log))
assert cases == {"Breacher": 40, "Module": 25, "Stance": 27}, cases
for name in ["Client", "Observer"]:
    assert "NR_BREACHER_NET_CLIENT PASSED" in read(root / f"Saved/Breacher{name}.log")
assert "BUILD SUCCESSFUL" in read(root / "Saved/MetaHumanPackage.log")
retarget = json.loads(read(root / "Saved/MetaHumanRetarget.json"))
assert len(retarget["assets"]) == 62 and not retarget["missing"]
old_file = release / "BUILDINFO.json"
old = json.loads(read(old_file)) if old_file.exists() else {}
if old and old.get("build") != "metahuman-breacher":
    shutil.copy2(old_file, folder / "PreviousBuildInfo.json")
manifest = {
    "build": "metahuman-breacher", "built_at": datetime.now().astimezone().isoformat(),
    "exe_sha256": sha(release / "NullRoute/Binaries/Win64/NullRoute.exe"),
    "checks": cases, "packaged_checks_total": sum(cases.values()),
    "body": "Epic MetaHuman Creator local body export: 342 bones, 3 LODs; 62 retargeted animation assets",
    "armor": "48 generated meshes: 15 bone attachments plus badge for each faction",
    "cloud_assembly": "INCOMPLETE: Epic authentication worked, S3 face-rig and texture downloads failed",
    "dedicated_two_clients": "PASS: Editor dedicated server, two clients, three factions and side swap",
    "scope": "Functional and visual prototype checks; no frame-rate or server-load benchmark",
    "previous_build_manifest": "MetaHuman/PreviousBuildInfo.json",
    "controls": old.get("controls", {}),
    "source_sha256": {str(p.relative_to(root)).replace("\\", "/"): sha(p) for p in [
        root / "Source/NRGameplay/Private/NRCharacterAppearance.cpp",
        root / "Source/NREditor/Private/NRMetaHumanAssetsCommandlet.cpp",
        root / "Source/NREditor/Private/NRMetaHumanArmorCommandlet.cpp",
        root / "Scripts/RetargetMetaHuman.py"]}
}
previews = root / "ArtSource/MetaHuman/Previews"
previews.mkdir(exist_ok=True)
for name in ["Chronos", "Police", "Rebel", "FirstPerson", "ADS", "Reload"]:
    source = release / f"NullRoute/Saved/Screenshots/Breacher_{name}.png"
    assert source.exists(), source
    shutil.copy2(source, previews / f"{name}.png")
    shutil.copy2(source, folder / f"{name}.png")
for target in [old_file, folder / "BUILDINFO.json", root / "ArtSource/MetaHuman/BUILDINFO.json"]:
    target.write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
shutil.copy2(root / "ArtSource/MetaHuman/README.md", folder / "README.md")
shutil.copy2(root / "ArtSource/MetaHuman/VERIFICATION.md", folder / "VERIFICATION.md")
print(json.dumps({"checks": cases, "total": sum(cases.values()), "exe_sha256": manifest["exe_sha256"]}))

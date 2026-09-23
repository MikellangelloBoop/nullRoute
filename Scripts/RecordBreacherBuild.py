"""Record evidence for the packaged build; refuse to label failed tests as passed."""
import hashlib
import json
import re
import shutil
from datetime import datetime
from pathlib import Path

root = Path(__file__).resolve().parent.parent
release = root/'Releases/Windows'
folder = release/'Breacher'
folder.mkdir(exist_ok=True)
cases = {}
for case in ['Breacher', 'Module', 'Stance']:
    log = (root/f'Saved/PackagedBreacher-{case}.log').read_text(encoding='utf-8-sig', errors='replace')
    assert f'NR_{case.upper()}_COMPLETE PASSED' in log, case
    cases[case] = len(re.findall(rf'LogTemp: Display: NR_{case.upper()} .+ : PASS',log))
for name in ['Client','Observer']:
    assert 'NR_BREACHER_NET_CLIENT PASSED' in (root/f'Saved/Breacher{name}.log').read_text(encoding='utf-8-sig',errors='replace')
assert 'BUILD SUCCESSFUL' in (root/'Saved/Package.log').read_text(encoding='utf-8-sig',errors='replace')
old_file = release/'BUILDINFO.json'
old = json.loads(old_file.read_text(encoding='utf-8-sig')) if old_file.exists() else {}
if old and old.get('build') != 'breacher-factions':
    shutil.copy2(old_file, folder/'PreviousBuildInfo.json')
sha = lambda p: hashlib.sha256(p.read_bytes()).hexdigest()
manifest = {
    'build': 'breacher-factions', 'built_at': datetime.now().astimezone().isoformat(),
    'exe_sha256': sha(release/'NullRoute/Binaries/Win64/NullRoute.exe'),
    'checks': cases, 'packaged_checks_total': sum(cases.values()),
    'dedicated_two_clients': 'PASS: Editor dedicated server, two clients, three factions and side swap',
    'screenshots': 'Faction portraits rendered in packaged game; widescreen gameplay and menu in Editor game mode',
    'controls': old.get('controls', {}),
    'scope': '15 articulated armor parts per faction; appearance does not change damage, ammo, or collision. Not a frame-rate or server-load benchmark.',
    'previous_build_manifest': 'Breacher/PreviousBuildInfo.json',
    'source_sha256': {str(p.relative_to(root)).replace('\\','/'): sha(p) for p in [
        root/'Source/NRGameplay/Private/NRCharacterAppearance.cpp',root/'Source/NRGameplay/Private/NRFactions.cpp',
        root/'Source/NRGameplay/Private/NRGameMode.cpp',root/'Source/NREditor/Private/NRBreacherAssetsCommandlet.cpp',
        root/'Source/NREditor/Private/NRMeshMaker.cpp',root/'Config/DefaultGame.ini']}
}
assert sum(cases.values()) == 84, cases
for target in [old_file, folder/'BUILDINFO.json']:
    target.write_text(json.dumps(manifest,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
for name in ['Chronos','Police','Rebel']:
    source = release/f'NullRoute/Saved/Screenshots/Breacher_{name}.png'
    assert source.exists()
    shutil.copy2(source,root/f'ArtSource/Breacher/Previews/Breacher_{name}.png')
    shutil.copy2(source,folder/f'{name}.png')
shutil.copy2(root/'ArtSource/Breacher/VERIFICATION.md',folder/'VERIFICATION.md')
print(json.dumps({'checks':cases,'total':sum(cases.values()),'exe_sha256':manifest['exe_sha256']}))

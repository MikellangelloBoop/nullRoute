"""Record a tested local release and copy its unmodified game screenshots."""
from pathlib import Path
import hashlib, json, re, shutil, struct
from datetime import datetime, timezone

root=Path(__file__).resolve().parents[1]
release=root/'Releases/Operators/Windows'
out=root/'ArtSource/Operators'
def log(name): return (root/'Saved'/name).read_text(encoding='utf-8-sig',errors='replace')
def sha(p):
 h=hashlib.sha256()
 with p.open('rb') as f:
  for chunk in iter(lambda:f.read(1024*1024),b''):h.update(chunk)
 return h.hexdigest()

assert 'BUILD SUCCESSFUL' in log('RosterPackage.log')
assert 'Result: Succeeded' in log('RosterHUDGameBuild.log')
assert 'NR_ROSTER_COMPLETE PASSED' in log('Packaged-NRRosterTest.log')
checks={'Roster':len(re.findall(r'NR_ROSTER .* : PASS',log('Packaged-NRRosterTest.log')))}
assert checks['Roster']==159
for key,count in [('Breacher',40),('Module',25),('Stance',27)]:
 text=log(f'PackagedBreacher-{key}.log')
 assert f'NR_{key.upper()}_COMPLETE PASSED' in text
 assert len(re.findall(rf'NR_{key.upper()} .* : PASS',text))==count
 checks[key]=count
for key in ['Client','Observer']:assert 'NR_ROSTER_NET_CLIENT PASSED' in log(f'RosterNetwork-{key}.log')
assert 'cosmetics-free PASS' in log('RosterNetwork-Server.log')
assert 'treatment RPC and replicated health PASS' in log('RosterNetwork-Server.log')
assert 'NR_ROSTER_VISUAL_COMPLETE' in log('Packaged-NRRosterVisual.log')
previews=out/'Previews';previews.mkdir(parents=True,exist_ok=True)
images=[]
for key in ['Chronos','Police','Rebel','Ascended','RustHounds','MedicFP','Menu']:
 source=release/f'NullRoute/Saved/Screenshots/Roster_{key}.png'
 with source.open('rb') as f: header=f.read(24)
 assert header[:8]==b'\x89PNG\r\n\x1a\n'
 width,height=struct.unpack('>II',header[16:24])
 shutil.copy2(source,previews/source.name)
 images.append({'file':source.name,'width':width,'height':height,'sha256':sha(source)})
meshes=list((root/'Content/Art/Meshes').glob('SM_MH_*.uasset'));assert len(meshes)==335
sets=[]
for role in ['Engineer','Scout','Breacher','Medic']:
 for faction in ['Chronos','Police','Rebel','Ascended','RustHounds']:
  parts=list((root/'ArtSource/Breacher/Models').glob(f'SM_MH_{role}_{faction}_*.obj'))
  visible=[p for p in parts if not p.stem.endswith(('_Badge','_Sleeve'))]
  assert len(visible)==15
  triangles=sum(sum(line.startswith('f ') for line in p.open()) for p in visible)
  sets.append({'role':role,'faction':faction,'visible_parts':15,'armor_triangles':triangles})
payload=[release/'NullRoute/Binaries/Win64/NullRoute.exe',*sorted((release/'NullRoute/Content/Paks').iterdir())]
info={'recorded_utc':datetime.now(timezone.utc).isoformat(),'release':str(release),'checks':checks,'total_checks':sum(checks.values()),'network':'dedicated server + owner client + observer: all 20 appearances, side swap, treatment RPC, replicated health; no server cosmetics','metahuman':{'bones':342,'body_LODs':3,'retargeted_assets':62,'face_rig':'not assembled; Epic/S3 cloud download unavailable'},'static_meshes':335,'sets':sets,'screenshots':images,'payload':{str(p.relative_to(release)):{'bytes':p.stat().st_size,'sha256':sha(p)} for p in payload}}
(out/'BUILDINFO.json').write_text(json.dumps(info,ensure_ascii=False,indent=2),encoding='utf-8')
(out/'VERIFICATION.md').write_text('''# Проверка операторов — 20 сентября 2026

Итоговая Windows Development-сборка UE 5.8.1: `Releases/Operators/Windows`.
Ресурсы приготовлены и упакованы через UAT: `Saved/RosterPackage.log`,
`BUILD SUCCESSFUL`, 0 ошибок и 0 предупреждений cook. После cook исправлены
только нативные подсказки и отступы HUD; финальный EXE собран отдельно
(`Saved/RosterHUDGameBuild.log`), скопирован и проверен в этой поставке.
Пять архивов ресурсов побайтно совпадают с упакованным кандидатом.

**251 проверка PASS:** Roster 159, Breacher 40, Module 25, Stance 27.
Проверены все 20 комплектов, наличие ресурсов, кости, отсутствие косметических
коллизий, лимиты треугольников, анимационный скелет кистей, рукава,
сохранение боеприпасов и здоровья при смене внешности, вторичное оружие,
смена сторон, смерть, новый раунд и выбор класса. Лечение проверено через GAS:
себе, союзнику, максимум здоровья, восстановление, стены, дальность, враг,
мёртвый медик, другой класс и блокировка до начала боя.

Выделенный сервер и два клиента завершили сетевую проверку всех 20 обликов,
смены сторон, запроса лечения и репликации здоровья. Сервер не создаёт
косметические компоненты. Логи: `Saved/RosterNetwork-{Server,Client,Observer}.log`.

Визуально проверены пять групповых кадров (слева направо: Инженер,
Разведчик, Штурмовик, Медик), вид Медика от первого лица и меню.
Скриншоты сняты игрой, без редактирования. Это стилизованные процедурные
модели, не фотореалистичные скульпты. Проверки не являются измерением FPS
полного матча. Локальный MetaHuman-экспорт содержит тело и кисти, но не
завершённую UE Optimized сборку с облачным лицевым ригом.

Один параллельный прогон прежнего Module-теста не прошёл шаг `quick reload
begins`: тест с абсолютными временными порогами чувствителен к задержкам кадров.
Изолированный повтор с `t.MaxFPS 60` прошёл все 25 проверок. Исходный лог
сохранён в `Saved/RosterConcurrentModuleFailure.log`; итоговый — в
`Saved/PackagedBreacher-Module.log`. Игровая задержка смены оружия сохранена.

Контрольные суммы поставки, количество треугольников каждого комплекта и
размеры изображений находятся в `BUILDINFO.json`.
''',encoding='utf-8')
print(f'RECORDED {len(sets)} sets, {sum(checks.values())} checks, {len(images)} game screenshots')

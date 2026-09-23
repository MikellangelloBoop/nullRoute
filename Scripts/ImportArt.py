import unreal
from pathlib import Path

root = Path(unreal.Paths.project_dir()).resolve()
tasks = []
for folder, dest in [('Textures','/Game/Art/Textures'), ('Audio','/Game/Art/Audio')]:
    for source in (root/'ArtSource'/folder).glob('*'):
        if source.suffix.lower() not in ['.png','.wav']: continue
        task = unreal.AssetImportTask()
        task.filename = str(source)
        task.destination_path = dest
        task.automated = True
        task.replace_existing = True
        task.save = True
        tasks.append(task)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
for task in tasks:
    if not task.imported_object_paths:
        raise RuntimeError('Import failed: '+task.filename)
    unreal.log('NR_ART_IMPORTED '+str(task.imported_object_paths))
mesh = unreal.load_asset('/Game/Weapons/Rifle/Meshes/SM_Rifle')
unreal.log('NR_RIFLE_BOUNDS '+str(mesh.get_bounds()))
sk = unreal.load_asset('/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple')
unreal.log('NR_SKELETON '+sk.get_editor_property('skeleton').get_path_name())
unreal.log('NR_ART_IMPORT_COMPLETE')

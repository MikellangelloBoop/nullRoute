"""Import the original supplied patch photographs without raster edits."""
import unreal
from pathlib import Path

root = Path(unreal.Paths.project_dir()).resolve()
tasks = []
for faction in ['Chronos', 'Police', 'Rebel']:
    task = unreal.AssetImportTask()
    task.filename = str(root / 'ArtSource/Breacher/References' / f'T_Patch{faction}.jpg')
    task.destination_path = '/Game/Art/Textures'
    task.destination_name = f'T_Patch{faction}'
    task.automated = True
    task.replace_existing = True
    task.save = True
    tasks.append(task)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
for task in tasks:
    if not task.imported_object_paths:
        raise RuntimeError(f'Patch import failed: {task.filename}')
    for path in task.imported_object_paths:
        texture = unreal.load_asset(path)
        texture.set_editor_property('max_texture_size', 2048)
        unreal.EditorAssetLibrary.save_loaded_asset(texture)
unreal.log('NR_BREACHER_PATCH_IMPORT_COMPLETE')

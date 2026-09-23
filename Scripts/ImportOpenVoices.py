from pathlib import Path
import unreal
root=Path(unreal.Paths.project_dir()).resolve()
tasks=[]
for role in range(3):
    for cue in range(5):
        name=f'V_R{role}_{cue}'
        task=unreal.AssetImportTask();task.filename=str(root/'ArtSource/Audio'/f'{name}.wav');task.destination_path='/Game/Art/Audio'
        task.automated=True;task.replace_existing=True;task.save=True;tasks.append(task)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
for task in tasks:
    if len(task.imported_object_paths)!=1:raise RuntimeError('Voice import failed: '+task.filename)
    sound=unreal.load_asset(task.imported_object_paths[0])
    sound.set_editor_property('priority',85.0)
    sound.set_editor_property('volume',1.0)
    sound.set_editor_property('compression_quality',85)
    if not .5<sound.get_editor_property('duration')<15:raise RuntimeError('Voice duration invalid: '+task.filename)
    unreal.EditorAssetLibrary.save_loaded_asset(sound)
unreal.log('NR_OPEN_VOICES_IMPORTED 15 PASS')

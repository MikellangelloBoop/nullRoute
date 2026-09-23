from pathlib import Path
import unreal
root=Path(unreal.Paths.project_dir())
for name in ['S_Shotgun','S_LMG']:
    task=unreal.AssetImportTask();task.filename=str(root/'ArtSource/Audio'/f'{name}.wav');task.destination_path='/Game/Art/Audio';task.destination_name=name;task.automated=True;task.replace_existing=True;task.save=True
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    if not unreal.EditorAssetLibrary.does_asset_exist('/Game/Art/Audio/'+name):raise RuntimeError('Audio import failed: '+name)
unreal.log('NR_ARSENAL_AUDIO_COMPLETE')

from pathlib import Path
import unreal
root=Path(unreal.Paths.project_dir()).resolve()
for name in ['S_DroneCharge','S_DroneShot','S_DroneDown','S_HitConfirm','S_KillConfirm','S_TacticalPing']:
 t=unreal.AssetImportTask();t.filename=str(root/'ArtSource/Audio'/f'{name}.wav');t.destination_path='/Game/Art/Audio';t.automated=True;t.replace_existing=True;t.save=True
 unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([t])
 if not t.imported_object_paths:raise RuntimeError('Import failed: '+name)
 wave=unreal.load_asset('/Game/Art/Audio/'+name);wave.set_editor_property('priority',65.0 if name=='S_DroneCharge' else 50.0);unreal.EditorAssetLibrary.save_loaded_asset(wave)
unreal.log('NR_OPERATIONS_AUDIO_COMPLETE')

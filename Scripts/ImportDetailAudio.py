import unreal
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve()
tasks=[]
for source in (root/'ArtSource/Audio').glob('*.wav'):
    t=unreal.AssetImportTask();t.filename=str(source);t.destination_path='/Game/Art/Audio'
    t.automated=True;t.replace_existing=True;t.save=True;tasks.append(t)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
for t in tasks:
    if not t.imported_object_paths:raise RuntimeError('Import failed: '+t.filename)
tone=unreal.load_asset('/Game/Art/Audio/S_RoomTone')
tone.set_editor_property('looping',True)
unreal.EditorAssetLibrary.save_loaded_asset(tone)
unreal.log('NR_DETAIL_AUDIO_COMPLETE '+str(len(tasks)))

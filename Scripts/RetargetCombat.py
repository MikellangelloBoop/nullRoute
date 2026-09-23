"""Add missing third-person action clips without replacing previously authored animations."""
import unreal
paths=['/Game/Characters/Mannequins/Anims/Pistol/MM_Pistol_Reload',
       '/Game/Characters/Mannequins/Anims/Rifle/Jump/MM_Rifle_Jump_Start',
       '/Game/Characters/Mannequins/Anims/Death/MM_Death_Front_01']
inputs=unreal.IKRetargetBatchOperationInputs()
inputs.source_mesh=unreal.load_asset('/Game/Art/Meshes/SKM_OperatorBody')
inputs.target_mesh=unreal.load_asset('/Game/Art/MetaHuman/SKM_BreacherBody')
inputs.ik_retarget_asset=unreal.load_asset('/Game/Art/MetaHuman/RTG_Operator_Breacher')
inputs.prefix='MH_';inputs.target_path='/Game/Art/MetaHuman/Animations';inputs.include_referenced_assets=False;inputs.overwrite_existing_files=False
missing=[p for p in paths if not unreal.EditorAssetLibrary.does_asset_exist(inputs.target_path+'/MH_'+p.rsplit('/',1)[-1])]
if missing:
    inputs.assets_to_retarget=[unreal.EditorAssetLibrary.find_asset_data(p) for p in missing]
    unreal.IKRetargetBatchOperation.run_batch_retarget(inputs)
unreal.EditorAssetLibrary.save_directory(inputs.target_path,only_if_is_dirty=True,recursive=True)
for p in paths:
    if not unreal.EditorAssetLibrary.does_asset_exist(inputs.target_path+'/MH_'+p.rsplit('/',1)[-1]):raise RuntimeError('Retarget missing '+p)
unreal.log('NR_COMBAT_RETARGET_COMPLETE')

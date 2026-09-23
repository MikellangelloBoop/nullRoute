import unreal
paths = unreal.EditorAssetLibrary.list_assets('/Game/Art/Materials') + unreal.EditorAssetLibrary.list_assets('/Game/Materials')
for path in paths:
    mat = unreal.load_asset(path)
    if not isinstance(mat, unreal.Material) or 'Visor' in path:
        continue
    mat.set_editor_property('used_with_instanced_static_meshes', True)
    if 'Team' in path:
        mat.set_editor_property('used_with_skeletal_mesh', True)
    if 'Concrete' in path:
        mat.set_editor_property('used_with_geometry_collections', True)
    unreal.MaterialEditingLibrary.recompile_material(mat)
    unreal.EditorAssetLibrary.save_loaded_asset(mat)
unreal.log('NR_MATERIALS_PREPARED')
anim=unreal.load_asset('/Game/Characters/Mannequins/Anims/Rifle/MF_Rifle_Idle_ADS')
pose=unreal.AnimPoseExtensions.get_anim_pose_at_time(anim, 0.0, unreal.AnimPoseEvaluationOptions())
for bone in ['hand_r','hand_l','lowerarm_r','upperarm_r','head','spine_03']:
    value=unreal.AnimPoseExtensions.get_bone_pose(pose,bone,unreal.AnimPoseSpaces.WORLD)
    unreal.log('NR_POSE '+bone+' '+str(value))

import unreal
knife=unreal.load_asset('/Game/Art/Meshes/SM_RouteKnife')
knife.set_material(2,unreal.load_asset('/Game/Art/Materials/M_Ceramic'))
unreal.EditorAssetLibrary.save_loaded_asset(knife)
unreal.log('NR_KNIFE_FINISH_COMPLETE')

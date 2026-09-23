import unreal
material = unreal.load_asset("/Game/Art/MetaHuman/M_Undersuit")
errors = unreal.MaterialEditingLibrary.recompile_material(material)
unreal.EditorAssetLibrary.save_loaded_asset(material)
unreal.log("NR_METAHUMAN_MATERIAL_ERRORS " + str(errors))
if errors:
    raise RuntimeError(str(errors))

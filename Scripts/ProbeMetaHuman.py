"""Inspect the installed Creator API and create the editable Breacher source."""
import json
from pathlib import Path
import unreal

report = {}
sub = unreal.get_editor_subsystem(unreal.MetaHumanCharacterEditorSubsystem)
path = "/Game/MetaHumans/Source/MHC_Breacher"
character = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
if not character:
    character = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "MHC_Breacher", "/Game/MetaHumans/Source", unreal.MetaHumanCharacter,
        unreal.MetaHumanCharacterFactoryNew())
if not character:
    raise RuntimeError("MetaHuman Creator did not create the source character")
report["character"] = character.get_path_name()
report["character_api"] = [x for x in dir(character) if not x.startswith("_")]
report["subsystem_api"] = [x for x in dir(sub) if not x.startswith("_")]
report["editable"] = sub.try_add_object_to_edit(character)
report["can_build"] = sub.can_build_meta_human(character)
for prop in ["body_type", "body_mesh", "face_mesh", "body_parameters", "has_high_resolution_textures", "rig_state", "head_mesh"]:
    try:
        report[prop] = str(character.get_editor_property(prop))
    except Exception as exc:
        report[prop] = str(exc)
for name in ["get_body_mesh", "get_face_mesh", "get_body_parameters", "set_body_parameters", "apply_body_preset", "apply_face_preset", "request_auto_rigging"]:
    if hasattr(sub, name):
        report[name] = str(getattr(sub, name).__doc__)
unreal.EditorAssetLibrary.save_loaded_asset(character)
if sub.is_object_added_for_editing(character):
    sub.remove_object_to_edit(character)
Path(unreal.Paths.project_saved_dir(), "MetaHumanProbe.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
unreal.log("NR_METAHUMAN_PROBE_COMPLETE")

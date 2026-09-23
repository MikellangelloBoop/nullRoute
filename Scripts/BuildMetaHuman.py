"""Build the real MetaHuman source using Epic's Creator and UE Optimized pipeline.

Run with UnrealEditor-Cmd -run=pythonscript -script=... -RenderOffscreen.
Auto-rigging and texture download use Epic's normal authenticated Creator service.
"""
import json
from pathlib import Path
import unreal

root = Path(unreal.Paths.project_saved_dir())
report = {"source_preset": "/MetaHumanCharacter/Optional/Presets/Dominic", "pipeline": "UE Optimized", "quality": "Medium"}
def record(stage):
    report["stage"] = stage
    (root / "MetaHumanBuild.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
    unreal.log("NR_METAHUMAN " + stage)

sub = unreal.get_editor_subsystem(unreal.MetaHumanCharacterEditorSubsystem)
path = "/Game/MetaHumans/Source/Breacher"
character = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
if not character:
    preset = unreal.load_asset(report["source_preset"])
    if not preset:
        raise RuntimeError("Cannot load the installed MetaHuman preset")
    character = unreal.AssetToolsHelpers.get_asset_tools().duplicate_asset("Breacher", "/Game/MetaHumans/Source", preset)
    if not character:
        raise RuntimeError("Cannot duplicate the installed MetaHuman preset")
report["character"] = character.get_path_name()
record("source_created")
if not sub.try_add_object_to_edit(character):
    raise RuntimeError("Cannot open Breacher in MetaHuman Creator")
try:
    constraints = sub.get_body_constraints(character)
    report["constraints"] = [{"name": str(c.name), "target": c.target_measurement, "active": c.is_active} for c in constraints]
    physique = {"height": 185.0, "chest": 110.0, "waist": 88.0,
                "across shoulder": 44.0, "fat": 0.16, "muscularity": 0.28}
    for c in constraints:
        if str(c.name).lower() in physique:
            c.is_active = True
            c.target_measurement = physique[str(c.name).lower()]
    sub.set_body_constraints(character, constraints)
    sub.commit_body_state(character)
    unreal.EditorAssetLibrary.save_loaded_asset(character)
    record("body_shaped")
    # These are the supported Creator calls; a failed service request must not be
    # passed off as a completed MetaHuman assembly.
    params = unreal.MetaHumanCharacterAutoRiggingRequestParams()
    params.blocking = True
    params.report_progress = False
    params.rig_type = unreal.MetaHumanRigType.JOINTS_ONLY
    record("requesting_rig")
    sub.request_auto_rigging(character, params)
    unreal.EditorAssetLibrary.save_loaded_asset(character)
    record("requesting_textures")
    textures = unreal.MetaHumanCharacterTextureRequestParams()
    textures.blocking = True
    textures.report_progress = False
    sub.request_texture_sources(character, textures)
    unreal.EditorAssetLibrary.save_loaded_asset(character)
    report["has_high_resolution_textures"] = character.has_high_resolution_textures
    report["can_build"] = sub.can_build_meta_human(character, True)
    if not report["can_build"]:
        record("creator_service_required")
        raise RuntimeError("MetaHuman rig or source textures unavailable; see Creator service errors")
    record("assembling")
    build = unreal.MetaHumanCharacterEditorBuildParameters()
    build.pipeline_type = unreal.MetaHumanDefaultPipelineType.OPTIMIZED
    build.pipeline_quality = unreal.MetaHumanQualityLevel.MEDIUM
    build.absolute_build_path = "/Game/MetaHumans"
    build.common_folder_path = "/Game/MetaHumans/Common"
    build.enable_wardrobe_item_validation = False
    sub.build_meta_human(character, build)
    unreal.EditorAssetLibrary.save_directory("/Game/MetaHumans", only_if_is_dirty=True, recursive=True)
    report["assembled_assets"] = unreal.EditorAssetLibrary.list_assets("/Game/MetaHumans/Breacher", recursive=True)
    if not report["assembled_assets"]:
        raise RuntimeError("Creator returned without assembled assets")
    record("assembled")
finally:
    if sub.is_object_added_for_editing(character):
        sub.remove_object_to_edit(character)

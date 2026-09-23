"""Bake the existing gameplay and viewmodel animations to the MetaHuman skeleton."""
import json
import shutil
from datetime import datetime
from pathlib import Path
import unreal

tools = unreal.AssetToolsHelpers.get_asset_tools()
source = unreal.load_asset("/Game/Art/Meshes/SKM_OperatorBody")
target = unreal.load_asset("/Game/Art/MetaHuman/SKM_BreacherBody")
if not source or not target:
    raise RuntimeError("Export the MetaHuman body before retargeting")

# UE 5.8's batch overwrite can leave a numbered asset while failing to replace a
# referenced blendspace. Rebuild this generated-only directory from a clean state.
animation_dir = "/Game/Art/MetaHuman/Animations"
existing = unreal.EditorAssetLibrary.list_assets(animation_dir, recursive=True, include_folder=False)
if existing:
    if any(not path.rsplit("/", 1)[-1].startswith("MH_") for path in existing):
        raise RuntimeError("Refusing to replace non-generated MetaHuman animations")
    project_root = Path(unreal.Paths.project_dir()).resolve()
    physical = (project_root / "Content/Art/MetaHuman/Animations").resolve()
    if not physical.is_relative_to(project_root / "Content/Art/MetaHuman"):
        raise RuntimeError("Generated animation directory escaped the project")
    backup = project_root / "Saved/MetaHumanAnimationBackups" / datetime.now().strftime("%Y%m%d-%H%M%S")
    shutil.copytree(physical, backup)
    if not unreal.EditorAssetLibrary.delete_directory(animation_dir):
        raise RuntimeError("Could not clear generated animations; backup retained at " + str(backup))

def get_or_create(name, cls, factory):
    path = "/Game/Art/MetaHuman/" + name
    asset = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else None
    return asset or tools.create_asset(name, "/Game/Art/MetaHuman", cls, factory)

rigs = []
for name, mesh in [("IK_Operator", source), ("IK_Breacher", target)]:
    rig = get_or_create(name, unreal.IKRigDefinition, unreal.IKRigDefinitionFactory())
    ctl = unreal.IKRigController.get_controller(rig)
    ctl.set_skeletal_mesh(mesh)
    if not ctl.apply_auto_generated_retarget_definition():
        raise RuntimeError("Epic could not characterize " + mesh.get_name())
    ctl.apply_auto_fbik()
    unreal.EditorAssetLibrary.save_loaded_asset(rig)
    rigs.append(rig)

retarget = get_or_create("RTG_Operator_Breacher", unreal.IKRetargeter, unreal.IKRetargetFactory())
ctl = unreal.IKRetargeterController.get_controller(retarget)
ctl.set_ik_rig(unreal.RetargetSourceOrTarget.SOURCE, rigs[0])
ctl.set_ik_rig(unreal.RetargetSourceOrTarget.TARGET, rigs[1])
ctl.set_preview_mesh(unreal.RetargetSourceOrTarget.SOURCE, source)
ctl.set_preview_mesh(unreal.RetargetSourceOrTarget.TARGET, target)
ctl.remove_all_ops()
ctl.add_default_ops()
ctl.auto_map_chains(unreal.AutoMapChainType.FUZZY, True)
ctl.auto_align_all_bones(unreal.RetargetSourceOrTarget.TARGET)
unreal.EditorAssetLibrary.save_loaded_asset(retarget)

paths = ["/Game/Art/Animations/" + name for name in [
    "BS_Operator2D", "BS_OperatorCrouch", "BS_OperatorPistol",
    "A_FP_Ready", "A_FP_Reload", "A_FP_Pistol", "A_FP_PistolReload", "A_FP_Knife"]]
paths += ["/Game/Characters/Mannequins/Anims/Rifle/MM_Rifle_Reload",
          "/Game/Characters/Mannequins/Anims/Rifle/Jump/MM_Rifle_Jump_Fall_Loop",
          "/Game/Characters/Mannequins/Anims/Unarmed/MM_Idle"]
inputs = unreal.IKRetargetBatchOperationInputs()
inputs.assets_to_retarget = [unreal.EditorAssetLibrary.find_asset_data(unreal.load_asset(p).get_path_name()) for p in paths]
inputs.source_mesh = source
inputs.target_mesh = target
inputs.ik_retarget_asset = retarget
inputs.prefix = "MH_"
inputs.target_path = animation_dir
inputs.include_referenced_assets = True
inputs.overwrite_existing_files = False
assets = unreal.IKRetargetBatchOperation.run_batch_retarget(inputs)
unreal.EditorAssetLibrary.save_directory("/Game/Art/MetaHuman", only_if_is_dirty=True, recursive=True)
missing = [p for p in paths if not unreal.EditorAssetLibrary.does_asset_exist(inputs.target_path + "/MH_" + p.rsplit("/", 1)[-1])]
report = {"source": source.get_path_name(), "target": target.get_path_name(), "retargeter": retarget.get_path_name(),
          "assets": [str(a.package_name) for a in assets], "missing": missing}
Path(unreal.Paths.project_saved_dir(), "MetaHumanRetarget.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
if missing:
    raise RuntimeError("Missing required retargeted animations: " + str(missing))
unreal.log("NR_METAHUMAN_RETARGET_COMPLETE " + str(len(assets)))

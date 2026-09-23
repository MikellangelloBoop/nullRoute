using UnrealBuildTool;
public class NREditor: ModuleRules { public NREditor(ReadOnlyTargetRules Target):base(Target) {
PCHUsage=PCHUsageMode.UseExplicitOrSharedPCHs;
PublicDependencyModuleNames.AddRange(new string[]{"Core","CoreUObject","Engine","UnrealEd","AssetRegistry","NRCore","NRGameplay","NRPhysics","GeometryCollectionEngine","Chaos","FieldSystemEngine","NavigationSystem","AIModule","MaterialEditor","MeshDescription","StaticMeshDescription","SkeletalMeshDescription","AnimationCore"});
PrivateDependencyModuleNames.AddRange(new string[]{"MetaHumanCharacter", "MetaHumanCharacterEditor", "IKRig", "IKRigEditor"});
} }

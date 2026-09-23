using UnrealBuildTool;
public class NRPhysics: ModuleRules { public NRPhysics(ReadOnlyTargetRules Target):base(Target) {
PCHUsage=PCHUsageMode.UseExplicitOrSharedPCHs;
PublicDependencyModuleNames.AddRange(new string[]{"Core","CoreUObject","Engine","NRCore","NRNetworking","GeometryCollectionEngine","FieldSystemEngine","Chaos","PhysicsCore","NavigationSystem","AIModule"});
} }

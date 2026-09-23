using UnrealBuildTool;
public class NRGameplay: ModuleRules { public NRGameplay(ReadOnlyTargetRules Target):base(Target) {
PCHUsage=PCHUsageMode.UseExplicitOrSharedPCHs;
PublicDependencyModuleNames.AddRange(new string[]{"Core","CoreUObject","Engine","DeveloperSettings","InputCore","NetCore","GameplayAbilities","GameplayTags","GameplayTasks","NRCore","NRNetworking","NRPhysics","NavigationSystem","AIModule","MassEntity","MassCommon","MassCore","AnimGraphRuntime","Slate","SlateCore"});
} }

using UnrealBuildTool;
public class NRCore: ModuleRules { public NRCore(ReadOnlyTargetRules Target):base(Target) {
PCHUsage=PCHUsageMode.UseExplicitOrSharedPCHs;
PublicDependencyModuleNames.AddRange(new string[]{"Core","CoreUObject","Engine","GameplayTags"});
} }

using UnrealBuildTool;
public class NRNetworking: ModuleRules { public NRNetworking(ReadOnlyTargetRules Target):base(Target) {
PCHUsage=PCHUsageMode.UseExplicitOrSharedPCHs;
PublicDependencyModuleNames.AddRange(new string[]{"Core","CoreUObject","Engine","NetCore","IrisCore","NRCore","Sockets","Networking","Json"});
SetupIrisSupport(Target);
} }

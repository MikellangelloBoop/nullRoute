using UnrealBuildTool;
public class NullRouteServerTarget : TargetRules {
public NullRouteServerTarget(TargetInfo Target) : base(Target) {
Type=TargetType.Server; DefaultBuildSettings=BuildSettingsVersion.Latest;
IncludeOrderVersion=EngineIncludeOrderVersion.Latest;
ExtraModuleNames.Add("NRGameplay");

} }

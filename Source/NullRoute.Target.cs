using UnrealBuildTool;
public class NullRouteTarget : TargetRules {
public NullRouteTarget(TargetInfo Target) : base(Target) {
Type=TargetType.Game; DefaultBuildSettings=BuildSettingsVersion.Latest;
IncludeOrderVersion=EngineIncludeOrderVersion.Latest;
ExtraModuleNames.Add("NRGameplay");

} }

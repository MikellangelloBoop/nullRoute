using UnrealBuildTool;
public class NullRouteEditorTarget : TargetRules {
public NullRouteEditorTarget(TargetInfo Target) : base(Target) {
Type=TargetType.Editor; DefaultBuildSettings=BuildSettingsVersion.Latest;
IncludeOrderVersion=EngineIncludeOrderVersion.Latest;
ExtraModuleNames.Add("NRGameplay");
ExtraModuleNames.Add("NREditor");
} }

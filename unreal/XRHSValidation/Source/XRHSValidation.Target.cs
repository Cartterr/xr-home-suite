using UnrealBuildTool;

public class XRHSValidationTarget : TargetRules
{
	public XRHSValidationTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("XRHSValidation");
	}
}

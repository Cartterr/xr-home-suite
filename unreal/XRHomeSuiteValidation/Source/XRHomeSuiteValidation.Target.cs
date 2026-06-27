using UnrealBuildTool;

public class XRHomeSuiteValidationTarget : TargetRules
{
	public XRHomeSuiteValidationTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("XRHomeSuiteValidation");
	}
}

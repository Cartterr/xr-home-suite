using UnrealBuildTool;

public class XRHSValidationEditorTarget : TargetRules
{
	public XRHSValidationEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.AddRange(new[] { "XRHSValidation", "XRHSValidationEditor" });
	}
}

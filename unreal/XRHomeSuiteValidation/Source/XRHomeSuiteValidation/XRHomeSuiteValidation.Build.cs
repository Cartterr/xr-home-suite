using UnrealBuildTool;

public class XRHomeSuiteValidation : ModuleRules
{
	public XRHomeSuiteValidation(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(
			new[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"HeadMountedDisplay",
				"InputCore",
				"OculusXRHMD",
				"OculusXRPassthrough",
				"RHI",
				"XRBase"
			});
	}
}

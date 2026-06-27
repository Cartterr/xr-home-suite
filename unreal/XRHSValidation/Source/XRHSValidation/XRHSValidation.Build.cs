using UnrealBuildTool;

public class XRHSValidation : ModuleRules
{
	public XRHSValidation(ReadOnlyTargetRules Target) : base(Target)
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

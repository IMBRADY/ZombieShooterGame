using UnrealBuildTool;

public class ZombieGame : ModuleRules
{
	public ZombieGame(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Lets any file in the module reach another folder's header via a module-root-relative
		// path (e.g. "Components/HealthComponent.h") instead of fragile "../../" chains — UE5's
		// IWYU-style builds only add a file's own folder to the include path by default.
		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"GameplayTags",
			"UMG",
			"Niagara"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });
	}
}

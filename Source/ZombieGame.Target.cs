using UnrealBuildTool;
using System.Collections.Generic;

public class ZombieGameTarget : TargetRules
{
	public ZombieGameTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("ZombieGame");
	}
}

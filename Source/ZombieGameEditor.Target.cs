using UnrealBuildTool;
using System.Collections.Generic;

public class ZombieGameEditorTarget : TargetRules
{
	public ZombieGameEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("ZombieGame");
	}
}

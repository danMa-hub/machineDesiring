// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MachineDesiring : ModuleRules
{
	public MachineDesiring(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "AIModule",  "NavigationSystem" });

		PublicIncludePaths.AddRange(new string[]
        {
            "MachineDesiring/Population",
            "MachineDesiring/NPC",
            "MachineDesiring/Simulation",
            "MachineDesiring/Core",
            "MachineDesiring/Overlay",
        });

		PrivateDependencyModuleNames.AddRange(new string[] { "UMG", "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}

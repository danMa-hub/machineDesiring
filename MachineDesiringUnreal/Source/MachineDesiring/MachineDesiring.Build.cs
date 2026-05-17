// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MachineDesiring : ModuleRules
{
	public MachineDesiring(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"NavigationSystem",

			// ── Mutable (CustomizableObjects) ─────────────────────────
			// Abilita dopo aver installato il plugin Mutable in UE5:
			//   Edit → Plugins → cerca "Mutable" → Enable → riavvia
			// Poi decommentare le righe seguenti:
			// "MutableRuntime",
			// "CustomizableObject",
		});

		PublicIncludePaths.AddRange(new string[]
		{
			"MachineDesiring/Population",
			"MachineDesiring/NPC",
			"MachineDesiring/Simulation",
			"MachineDesiring/Core",
			"MachineDesiring/Overlay",
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"UMG",
			"Slate",
			"SlateCore",
		});
	}
}

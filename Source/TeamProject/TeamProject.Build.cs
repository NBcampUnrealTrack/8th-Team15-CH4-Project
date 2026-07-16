// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class TeamProject : ModuleRules
{
	public TeamProject(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { 
			// Default
			"Core", "CoreUObject", "Engine", 
			// Input
			"InputCore", "EnhancedInput", 
			// UI
			"UMG", "Slate", "SlateCore",
			// Online
			"OnlineSubsystem", "OnlineSubsystemUtils",
        });
		
		// Null Subsystem Module
		DynamicallyLoadedModuleNames.Add("OnlineSubsystemNull");

		PrivateDependencyModuleNames.AddRange(new string[] {
            "InteractiveToolsFramework"
        });

        PublicIncludePaths.AddRange(new string[] { "TeamProject" });
        // Uncomment if you are using Slate UI
        // PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
    }
}

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
		
		// Online Subsystem 구현체 — 어느 쪽을 쓸지는 DefaultEngine.ini의 DefaultPlatformService가 정한다.
		// 둘 다 실어두는 이유: Steam이 없는 환경(스팀 미실행/미설치)에서 엔진이 Null로 폴백할 수 있어야 함.
		DynamicallyLoadedModuleNames.Add("OnlineSubsystemNull");
		DynamicallyLoadedModuleNames.Add("OnlineSubsystemSteam");

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

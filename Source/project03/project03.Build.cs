// Fill out your copyright notice in the Description page of Project Settings.

using UnrealBuildTool;

public class project03 : ModuleRules
{
	public project03(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "InputCore", "NavigationSystem" });
        
        //if (Target.bBuildEditor)
        //{
        //    PrivateDependencyModuleNames.AddRange(new string[]
        //    {

        //        //"UnrealEd",              // 에디터 관련 모듈

        //        //"EditorSubsystem",        // EditorSubsystem 사용 시 필요
        //        "Slate",                  // UI용 모듈 (사용하는 경우)
        //        "SlateCore",              // UI용 모듈 (사용하는 경우)
        //        //"LevelEditor",            // 레벨 에디터 관련 모듈
        //        "MainFrame",              // 메인 프레임 관련 모듈
        //        "RenderCore",             // 텍스처 빌드 관련 모듈
        //        "TextureCompressor"       // 텍스처 빌드 관련 모듈
        //    });
        //}


        PrivateDependencyModuleNames.AddRange(new string[] {  });

        // Uncomment if you are using Slate UI
        PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

        // Uncomment if you are using online features
        // PrivateDependencyModuleNames.Add("OnlineSubsystem");

        // To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
    }
}

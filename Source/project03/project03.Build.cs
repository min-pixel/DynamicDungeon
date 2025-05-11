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

        //        //"UnrealEd",              // ������ ���� ���

        //        //"EditorSubsystem",        // EditorSubsystem ��� �� �ʿ�
        //        "Slate",                  // UI�� ��� (����ϴ� ���)
        //        "SlateCore",              // UI�� ��� (����ϴ� ���)
        //        //"LevelEditor",            // ���� ������ ���� ���
        //        "MainFrame",              // ���� ������ ���� ���
        //        "RenderCore",             // �ؽ�ó ���� ���� ���
        //        "TextureCompressor"       // �ؽ�ó ���� ���� ���
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

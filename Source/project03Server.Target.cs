using UnrealBuildTool;
using System.Collections.Generic;

public class project03ServerTarget : TargetRules
{
    public project03ServerTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Server;                  // �� Server Ÿ�� ����
        DefaultBuildSettings = BuildSettingsVersion.V5;


        CppStandard = CppStandardVersion.Cpp20;

        ExtraModuleNames.Add("project03");         // ���� ��� �̸�
    }
}

using UnrealBuildTool;
using System.Collections.Generic;

public class project03ServerTarget : TargetRules
{
    public project03ServerTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Server;                  // ← Server 타입 지정
        DefaultBuildSettings = BuildSettingsVersion.V2;
        ExtraModuleNames.Add("project03");         // 게임 모듈 이름
    }
}

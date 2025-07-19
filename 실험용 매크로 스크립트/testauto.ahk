#NoEnv
#SingleInstance Force
SendMode Input
SetWorkingDir %A_ScriptDir%

ExperimentCount := 50
WaitTime := 25000
CleanupTime := 3000
UnrealWindowTitle := "project03 - 언리얼 에디터"

MsgBox, 4, WFC 자동화 시작, %ExperimentCount%번의 WFC 실험을 자동으로 실행합니다.`n`n계속하시겠습니까?
IfMsgBox No
{
   ExitApp
}

Loop, %ExperimentCount% {
   CurrentExperiment := A_Index
   
   WinActivate, %UnrealWindowTitle%
   WinWaitActive, %UnrealWindowTitle%, , 5
   
   if ErrorLevel {
       MsgBox, 16, 오류, 언리얼 에디터 창을 찾을 수 없습니다!`n창 제목: %UnrealWindowTitle%
       ExitApp
   }
   
   FormatTime, CurrentTime, , HH:mm:ss
   ToolTip, [%CurrentTime%] 실험 %CurrentExperiment%/%ExperimentCount% 실행중..., 10, 10
   
   Send, {F5}
   Sleep, 1000
   
   Sleep, %WaitTime%
   
   Send, {Esc}
   Sleep, 500
   
   Sleep, %CleanupTime%
   
   if (Mod(CurrentExperiment, 10) = 0) {
       Progress := Round((CurrentExperiment / ExperimentCount) * 100, 1)
       MsgBox, 1, 진행상황, %CurrentExperiment%/%ExperimentCount% 완료 (%Progress%`%), 5
   }
}

MsgBox, 64, 완료, %ExperimentCount%번의 실험이 모두 완료되었습니다!
ExitApp

^+q::ExitApp
F10::Pause, Toggle
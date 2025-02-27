// Fill out your copyright notice in the Description page of Project Settings.


#include "DynamicDungeonModeBase.h"
#include "MyDCharacter.h"

ADynamicDungeonModeBase::ADynamicDungeonModeBase() {
	//C++ 클래스는 StaticClass() 사용 (블루프린트가 아니므로 경로 찾기 불필요)
	DefaultPawnClass = AMyDCharacter::StaticClass();

	//플레이어 컨트롤러가 없으면 움직일 수 없으므로 명시적으로 설정
	PlayerControllerClass = APlayerController::StaticClass();
}


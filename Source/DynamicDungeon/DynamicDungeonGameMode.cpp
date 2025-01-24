// Copyright Epic Games, Inc. All Rights Reserved.

#include "DynamicDungeonGameMode.h"
#include "DynamicDungeonCharacter.h"
#include "UObject/ConstructorHelpers.h"

ADynamicDungeonGameMode::ADynamicDungeonGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

}

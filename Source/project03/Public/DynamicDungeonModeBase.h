// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "DynamicDungeonModeBase.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT03_API ADynamicDungeonModeBase : public AGameModeBase
{
	GENERATED_BODY()

public:
	ADynamicDungeonModeBase();


protected:
	virtual void BeginPlay() override;

};

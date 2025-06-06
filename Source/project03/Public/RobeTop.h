// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Armor.h"
#include "RobeTop.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT03_API ARobeTop : public AArmor
{
	GENERATED_BODY()
public:
	ARobeTop();

	virtual void BeginPlay() override;
};

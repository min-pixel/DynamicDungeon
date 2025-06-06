// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Armor.h"
#include "RobeBottom.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT03_API ARobeBottom : public AArmor
{
	GENERATED_BODY()
public:
	ARobeBottom();

	virtual void BeginPlay() override;
};

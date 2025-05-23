// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Armor.h"
#include "Legs.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT03_API ALegs : public AArmor
{
	GENERATED_BODY()
public:
	ALegs();

	virtual void BeginPlay() override;

	UPROPERTY()
	USkeletalMesh* LoadedMesh;

};

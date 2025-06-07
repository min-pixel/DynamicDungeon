// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Armor.h"
#include "Hat.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT03_API AHat : public AArmor
{
	GENERATED_BODY()
public:
	AHat();

	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* HatMeshComponent;

	virtual void BeginPlay() override;
};

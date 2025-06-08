// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Armor.h"
#include "Mask.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT03_API AMask : public AArmor
{
	GENERATED_BODY()
public:
	AMask();


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	UStaticMeshComponent* MaskMeshComponent;

	virtual void BeginPlay() override;
};

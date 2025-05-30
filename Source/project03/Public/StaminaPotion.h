// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Item.h"
#include "StaminaPotion.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT03_API AStaminaPotion : public AItem
{
	GENERATED_BODY()

public:
	AStaminaPotion();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Potion")
	int32 StaminaAmount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FItemData ItemData;
};

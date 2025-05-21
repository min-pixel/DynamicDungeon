// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Item.h"
#include "ManaPotion.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT03_API AManaPotion : public AItem
{
	GENERATED_BODY()
	
public:
	AManaPotion();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Potion")
	int32 ManaAmount;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
	FItemData ItemData;

};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Item.h"
#include "Potion.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT03_API APotion : public AItem
{
	GENERATED_BODY()
	
public:
    APotion();

protected:
    virtual void BeginPlay() override;

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Potion")
    int32 HealAmount;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item")
    FItemData ItemData;

    int32 GetHealAmount() const { return HealAmount; }

};

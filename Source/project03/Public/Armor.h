// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ItemDataD.h"
#include "Item.h"
#include "Armor.generated.h"

UENUM(BlueprintType)
enum class EArmorType : uint8
{
    None     UMETA(DisplayName = "None"),
    Helmet   UMETA(DisplayName = "Helmet"),
    Chest    UMETA(DisplayName = "Chest"),
    Legs     UMETA(DisplayName = "Legs")
};


/**
 * 
 */
UCLASS(Abstract)
class PROJECT03_API AArmor : public AItem
{
	GENERATED_BODY()

public:
    AArmor();

    // °©¿Ê ºÎÀ§
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armor")
    EArmorType ArmorType;

    // Ãß°¡ Ã¼·Â
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armor")
    int32 BonusHealth;

    UFUNCTION(BlueprintCallable, Category = "Armor")
    virtual void ApplyArmorStats(class AMyDCharacter* Character);

    UFUNCTION(BlueprintCallable, Category = "Armor")
    virtual void RemoveArmorStats(class AMyDCharacter* Character);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    USkeletalMesh* ArmorVisualMesh;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Armor")
    UStaticMesh* HelmetStaticMesh;

};

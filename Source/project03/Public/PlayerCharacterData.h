// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ItemDataD.h"
#include "PlayerCharacterData.generated.h"

UENUM(BlueprintType)
enum class EPlayerClass : uint8
{

    Mage        UMETA(DisplayName = "Mage"),
    Warrior     UMETA(DisplayName = "Warrior"),
    Rogue       UMETA(DisplayName = "Rogue")
    
};

USTRUCT(BlueprintType)
struct FPlayerCharacterData
{
    GENERATED_BODY()

    FPlayerCharacterData()
        : CharacterId(0)
        , PlayerName(TEXT("Player"))
        , PlayerClass(EPlayerClass::Mage)
        , MaxHealth(1000.f)
        , MaxStamina(500.f)
        , MaxKnowledge(30.f)
        , Level(1)
    {}

    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 CharacterId = 0;

    UPROPERTY(BlueprintReadWrite)
    FString PlayerName;

    UPROPERTY(BlueprintReadWrite)
    EPlayerClass PlayerClass;

    UPROPERTY(BlueprintReadWrite)
    float MaxHealth;

    UPROPERTY(BlueprintReadWrite)
    float MaxStamina;

    UPROPERTY(BlueprintReadWrite)
    float MaxKnowledge;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Currency")
    int32 Gold = 1000;

    UPROPERTY(BlueprintReadWrite)
    TArray<FItemData> InventoryItems;

    UPROPERTY(BlueprintReadWrite)
    TArray<FItemData> EquippedItems;

    UPROPERTY(BlueprintReadWrite)
    int32 Level;


};
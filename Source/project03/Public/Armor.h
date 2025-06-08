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

UENUM(BlueprintType)
enum class EArmorGrade : uint8
{
    C UMETA(DisplayName = "C"),
    B UMETA(DisplayName = "B"),
    A UMETA(DisplayName = "A")
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

    // 갑옷 부위
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armor")
    EArmorType ArmorType;

    // 추가 체력
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armor")
    int32 BonusHealth;

    // 추가 마나
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armor")
    int32 BonusMana;

    // 추가 스테니마
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armor")
    int32 BonusStamina;

    UFUNCTION(BlueprintCallable, Category = "Armor")
    virtual void ApplyArmorStats(class AMyDCharacter* Character);

    UFUNCTION(BlueprintCallable, Category = "Armor")
    virtual void RemoveArmorStats(class AMyDCharacter* Character);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Visual")
    USkeletalMeshComponent* ArmorVisualMesh;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Armor")
    UStaticMesh* HelmetStaticMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armor")
    EArmorGrade ArmorGrade;

    // 실제 적용된 보너스를 기억해둠 (제거 시 사용)
    int32 CachedAppliedBonus = 0;

    int32 StaminaBonus = 0;
    int32 ManaBonus = 0;

    // 등급별 머티리얼
    UPROPERTY(EditDefaultsOnly, Category = "Armor|Material")
    UMaterialInterface* SilverMaterial;

    UPROPERTY(EditDefaultsOnly, Category = "Armor|Material")
    UMaterialInterface* GoldMaterial;



};

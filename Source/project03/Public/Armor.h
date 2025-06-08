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

    // ���� ����
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armor")
    EArmorType ArmorType;

    // �߰� ü��
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armor")
    int32 BonusHealth;

    // �߰� ����
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Armor")
    int32 BonusMana;

    // �߰� ���״ϸ�
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

    // ���� ����� ���ʽ��� ����ص� (���� �� ���)
    int32 CachedAppliedBonus = 0;

    int32 StaminaBonus = 0;
    int32 ManaBonus = 0;

    // ��޺� ��Ƽ����
    UPROPERTY(EditDefaultsOnly, Category = "Armor|Material")
    UMaterialInterface* SilverMaterial;

    UPROPERTY(EditDefaultsOnly, Category = "Armor|Material")
    UMaterialInterface* GoldMaterial;



};

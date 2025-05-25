// Fill out your copyright notice in the Description page of Project Settings.

#include "Armor.h"
#include "MyDCharacter.h"


AArmor::AArmor()
{
    ItemType = EItemType::Armor;  // 모든 갑옷 공통
    ArmorGrade = EArmorGrade::A;
    BonusHealth = 0;
    ArmorVisualMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("ArmorVisualMesh"));
    SetRootComponent(ArmorVisualMesh);

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> SilverMatObj(TEXT("/Game/StarterContent/Materials/M_Metal_Steel.M_Metal_Steel"));
    if (SilverMatObj.Succeeded())
    {
        SilverMaterial = SilverMatObj.Object;
    }

    static ConstructorHelpers::FObjectFinder<UMaterialInterface> GoldMatObj(TEXT("/Game/StarterContent/Materials/M_Metal_Gold.M_Metal_Gold"));
    if (GoldMatObj.Succeeded())
    {
        GoldMaterial = GoldMatObj.Object;
    }

}

void AArmor::ApplyArmorStats(AMyDCharacter* Character)
{
    if (!Character) return;

    float Multiplier = 1.0f;
    switch (ArmorGrade)
    {
    case EArmorGrade::B:
        Multiplier = 1.25f;

        /// 실버 머티리얼 적용 (투구 제외)
        if (ArmorVisualMesh && SilverMaterial)
        {
            ArmorVisualMesh->SetMaterial(0, SilverMaterial);
        }
        

        break;
    case EArmorGrade::A:
        Multiplier = 1.5f;

        // 골드 머티리얼 적용 (투구 제외)
        if (ArmorVisualMesh && GoldMaterial)
        {
            ArmorVisualMesh->SetMaterial(0, GoldMaterial);
        }
        
        break;
    case EArmorGrade::C:

        
    default:
        break;
    }

    CachedAppliedBonus = FMath::RoundToInt(BonusHealth * Multiplier);

    Character->MaxHealth += CachedAppliedBonus;
    Character->Health = FMath::Clamp(Character->Health + CachedAppliedBonus, 0.0f, Character->MaxHealth);
}

void AArmor::RemoveArmorStats(AMyDCharacter* Character)
{
    if (!Character) return;

    Character->MaxHealth -= CachedAppliedBonus;
    Character->Health = FMath::Clamp(Character->Health, 0.0f, Character->MaxHealth);
}


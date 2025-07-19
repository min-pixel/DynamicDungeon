// Fill out your copyright notice in the Description page of Project Settings.

#include "Armor.h"
#include "MyDCharacter.h"


AArmor::AArmor()
{
    ItemType = EItemType::Armor;  // 모든 갑옷 공통
    ArmorGrade = EArmorGrade::A;
    BonusHealth = 0;
    BonusMana = 0;
    BonusStamina = 0;
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

    bReplicates = true;
    bAlwaysRelevant = true;


}

void AArmor::ApplyArmorStats(AMyDCharacter* Character)
{
    if (!Character) return;

    // 등급별 배수 계산
    float Multiplier = 1.0f;
    switch (ArmorGrade)
    {
    case EArmorGrade::B:
        Multiplier = 1.25f;
        break;
    case EArmorGrade::A:
        Multiplier = 1.5f;
        break;
    case EArmorGrade::C:
    default:
        Multiplier = 1.0f;
        break;
    }

    // 실제 적용될 보너스 계산 및 캐시 (이게 핵심!)
    CachedAppliedBonus = FMath::RoundToInt(BonusHealth * Multiplier);
    StaminaBonus = FMath::RoundToInt(BonusStamina * Multiplier);
    ManaBonus = FMath::RoundToInt(BonusMana * Multiplier);

    // 최대 스탯 증가
    Character->MaxHealth += CachedAppliedBonus;
    Character->MaxStamina += StaminaBonus;
    Character->MaxKnowledge += ManaBonus;

    
    Character->Health = FMath::Clamp(Character->Health , 0.0f, Character->MaxHealth);
    Character->Stamina = FMath::Clamp(Character->Stamina , 0.0f, Character->MaxStamina);
    Character->Knowledge = FMath::Clamp(Character->Knowledge , 0.0f, Character->MaxKnowledge);

    UE_LOG(LogTemp, Warning, TEXT("Applied armor stats (Grade %s, Multiplier %.2f) - Health: +%d (base %d), Stamina: +%d (base %d), Mana: +%d (base %d)"),
        *UEnum::GetValueAsString(ArmorGrade), Multiplier,
        CachedAppliedBonus, BonusHealth,
        StaminaBonus, BonusStamina,
        ManaBonus, BonusMana);

    Character->UpdateHUD();

}

void AArmor::RemoveArmorStats(AMyDCharacter* Character)
{
    if (!Character) return;

    UE_LOG(LogTemp, Warning, TEXT("Removing armor stats - Health: -%d, Stamina: -%d, Mana: -%d"),
        CachedAppliedBonus, StaminaBonus, ManaBonus);

    // 최대 스탯에서 정확히 적용되었던 보너스만큼 차감
    Character->MaxHealth -= CachedAppliedBonus;
    Character->MaxStamina -= StaminaBonus;
    Character->MaxKnowledge -= ManaBonus;

    // 현재 스탯이 새로운 최대값을 넘지 않도록 조정
    Character->Health = FMath::Clamp(Character->Health, 0.0f, Character->MaxHealth);
    Character->Stamina = FMath::Clamp(Character->Stamina, 0.0f, Character->MaxStamina);
    Character->Knowledge = FMath::Clamp(Character->Knowledge, 0.0f, Character->MaxKnowledge);

    UE_LOG(LogTemp, Warning, TEXT("After removal - MaxHealth: %f, Health: %f"),
        Character->MaxHealth, Character->Health);

    Character->UpdateHUD();

    // 캐시된 값들 초기화
    CachedAppliedBonus = 0;
    StaminaBonus = 0;
    ManaBonus = 0;
}


// Fill out your copyright notice in the Description page of Project Settings.

#include "Armor.h"
#include "MyDCharacter.h"


AArmor::AArmor()
{
    ItemType = EItemType::Armor;  // ��� ���� ����
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

    // ��޺� ��� ���
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

    // ���� ����� ���ʽ� ��� �� ĳ�� (�̰� �ٽ�!)
    CachedAppliedBonus = FMath::RoundToInt(BonusHealth * Multiplier);
    StaminaBonus = FMath::RoundToInt(BonusStamina * Multiplier);
    ManaBonus = FMath::RoundToInt(BonusMana * Multiplier);

    // �ִ� ���� ����
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

    // �ִ� ���ȿ��� ��Ȯ�� ����Ǿ��� ���ʽ���ŭ ����
    Character->MaxHealth -= CachedAppliedBonus;
    Character->MaxStamina -= StaminaBonus;
    Character->MaxKnowledge -= ManaBonus;

    // ���� ������ ���ο� �ִ밪�� ���� �ʵ��� ����
    Character->Health = FMath::Clamp(Character->Health, 0.0f, Character->MaxHealth);
    Character->Stamina = FMath::Clamp(Character->Stamina, 0.0f, Character->MaxStamina);
    Character->Knowledge = FMath::Clamp(Character->Knowledge, 0.0f, Character->MaxKnowledge);

    UE_LOG(LogTemp, Warning, TEXT("After removal - MaxHealth: %f, Health: %f"),
        Character->MaxHealth, Character->Health);

    Character->UpdateHUD();

    // ĳ�õ� ���� �ʱ�ȭ
    CachedAppliedBonus = 0;
    StaminaBonus = 0;
    ManaBonus = 0;
}


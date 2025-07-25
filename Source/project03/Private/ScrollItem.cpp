// Fill out your copyright notice in the Description page of Project Settings.


#include "ScrollItem.h"
#include "MyDCharacter.h"
#include "FireballSpell.h"
#include "HealSpell.h"
#include "CurseSpell.h"

TArray<TSubclassOf<UUSpellBase>> AScrollItem::SharedSpellPool;

void AScrollItem::InitializeSpellPoolIfNeeded()
{
    if (SharedSpellPool.Num() == 0)
    {
        SharedSpellPool.Add(UFireballSpell::StaticClass());
        SharedSpellPool.Add(UHealSpell::StaticClass());
        SharedSpellPool.Add(UCurseSpell::StaticClass());
        // ���ο� ���� cng�߰�
    }
}


AScrollItem::AScrollItem()
{
    ItemName = TEXT("Scroll");
    ItemType = EItemType::Consumable;
    Price = 150;
    // ���� ��ų �ε��� ����
    //SkillIndex = FMath::RandRange(0, 1);

    static ConstructorHelpers::FObjectFinder<UTexture2D> IconTexture(TEXT("/Game/BP/Icon/scroll.scroll"));
    if (IconTexture.Succeeded())
    {
        ItemIcon = IconTexture.Object;
    }

}

// ���� ����ü ��� ���
void AScrollItem::UseWithData(AMyDCharacter* Character, const FItemData& Data)
{
    TSubclassOf<UUSpellBase> SpellClass = GetSpellFromIndex(Data.SkillIndex);
    if (!Character || !SpellClass) return;

    UE_LOG(LogTemp, Warning, TEXT("=== SCROLL USED ==="));
    UE_LOG(LogTemp, Warning, TEXT("Player: %s, Class: %d, SkillIndex: %d"),
        *Character->GetName(), (int32)Character->PlayerClass, Data.SkillIndex);

    // Ÿ�� ��ġ ���
    FVector TargetLocation = FVector::ZeroVector;
    FRotator TargetRotation = Character->GetControlRotation();



    if (Data.SkillIndex == 0) // Fireball
    {
        TargetLocation = Character->GetActorLocation() + Character->GetActorForwardVector() * 200.0f + FVector(0, 0, 50.0f);
    }
    else if (Data.SkillIndex == 1) // Heal
    {
        TargetLocation = Character->GetActorLocation();
    }
    else if (Data.SkillIndex == 2) // Curse
    {
        FVector Start = Character->GetActorLocation() + Character->GetActorForwardVector() * 100.0f;
        FVector End = Start + Character->GetActorForwardVector() * 1000.0f;
        TargetLocation = End;
    }

    // *** ��ũ�� ���� RPC ��� (����/Ŭ���� üũ ����) ***
    if (Character->HasAuthority())
    {
        Character->ServerCastScrollSpell(Data.SkillIndex, TargetLocation, TargetRotation);
    }
    else
    {
        Character->ServerCastScrollSpell(Data.SkillIndex, TargetLocation, TargetRotation);
    }

    // ���� ���� ȸ�� (���ʽ�)
   /* Character->Knowledge = Character->MaxKnowledge;
    Character->UpdateHUD();*/

    UE_LOG(LogTemp, Warning, TEXT("Scroll spell requested - any class can use!"));
}


void AScrollItem::Use_Implementation(AMyDCharacter* Character)
{
    UseWithData(Character, ToItemData());
}


TSubclassOf<UUSpellBase> AScrollItem::GetSpellFromIndex(int32 Index) const
{
    switch (Index)
    {
    case 0:
        return UFireballSpell::StaticClass();
    case 1:
        return UHealSpell::StaticClass();
    case 2:
        return UCurseSpell::StaticClass();
    default:
        return nullptr;
    }
}

void AScrollItem::InitFromData(const FItemData& Data)
{
    SkillIndex = Data.SkillIndex;
}
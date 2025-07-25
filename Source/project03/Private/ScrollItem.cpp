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
        // 새로운 마법 cng추가
    }
}


AScrollItem::AScrollItem()
{
    ItemName = TEXT("Scroll");
    ItemType = EItemType::Consumable;
    Price = 150;
    // 랜덤 스킬 인덱스 선택
    //SkillIndex = FMath::RandRange(0, 1);

    static ConstructorHelpers::FObjectFinder<UTexture2D> IconTexture(TEXT("/Game/BP/Icon/scroll.scroll"));
    if (IconTexture.Succeeded())
    {
        ItemIcon = IconTexture.Object;
    }

}

// 기존 구조체 기반 사용
void AScrollItem::UseWithData(AMyDCharacter* Character, const FItemData& Data)
{
    TSubclassOf<UUSpellBase> SpellClass = GetSpellFromIndex(Data.SkillIndex);
    if (!Character || !SpellClass) return;

    UE_LOG(LogTemp, Warning, TEXT("=== SCROLL USED ==="));
    UE_LOG(LogTemp, Warning, TEXT("Player: %s, Class: %d, SkillIndex: %d"),
        *Character->GetName(), (int32)Character->PlayerClass, Data.SkillIndex);

    // 타겟 위치 계산
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

    // *** 스크롤 전용 RPC 사용 (마나/클래스 체크 없음) ***
    if (Character->HasAuthority())
    {
        Character->ServerCastScrollSpell(Data.SkillIndex, TargetLocation, TargetRotation);
    }
    else
    {
        Character->ServerCastScrollSpell(Data.SkillIndex, TargetLocation, TargetRotation);
    }

    // 마나 완전 회복 (보너스)
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
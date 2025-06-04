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
    ItemType = EItemType::Consumable;

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

    UUSpellBase* Spell = NewObject<UUSpellBase>(Character, SpellClass);
    if (Spell && Spell->CanActivate(Character))
    {

        // 마법 시전 몽타주 재생
        if (Character)
        {
            UE_LOG(LogTemp, Warning, TEXT("Trying to play montage"));
            Character->PlayMagicMontage();
        }

        Spell->ActivateSpell(Character);
        Character->Knowledge = Character->MaxKnowledge;
    }
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
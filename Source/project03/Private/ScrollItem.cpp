// Fill out your copyright notice in the Description page of Project Settings.


#include "ScrollItem.h"
#include "MyDCharacter.h"
#include "FireballSpell.h"
#include "HealSpell.h"

AScrollItem::AScrollItem()
{
    ItemType = EItemType::Consumable;

    // ·£´ý ½ºÅ³ ÀÎµ¦½º ¼±ÅÃ
    SkillIndex = FMath::RandRange(0, 1);

    static ConstructorHelpers::FObjectFinder<UTexture2D> IconTexture(TEXT("/Game/BP/Icon/scroll.scroll"));
    if (IconTexture.Succeeded())
    {
        ItemIcon = IconTexture.Object;
    }

}


void AScrollItem::BeginPlay()
{
    Super::BeginPlay();

    TArray<TSubclassOf<UUSpellBase>> SpellPool;
    SpellPool.Add(UFireballSpell::StaticClass());
    SpellPool.Add(UHealSpell::StaticClass());

    int32 RandIndex = FMath::RandRange(0, SpellPool.Num() - 1);
    AssignedSpell = SpellPool[RandIndex];

    ItemName = TEXT("Scroll of ") + AssignedSpell->GetDefaultObject<UUSpellBase>()->SpellName.ToString();

}

void AScrollItem::Use_Implementation(AMyDCharacter* Character)
{
    TSubclassOf<UUSpellBase> SpellClass = GetSpellFromIndex(SkillIndex);
    if (!Character || !SpellClass) return;

    UUSpellBase* Spell = NewObject<UUSpellBase>(Character, SpellClass);
    if (Spell && Spell->CanActivate(Character))
    {
        Spell->ActivateSpell(Character);
        Character->Knowledge = Character->MaxKnowledge;
        //Character->UpdateHUD();
    }
}


TSubclassOf<UUSpellBase> AScrollItem::GetSpellFromIndex(int32 Index) const
{
    switch (Index)
    {
    case 0:
        return UFireballSpell::StaticClass();
    case 1:
        return UHealSpell::StaticClass();
        // case 2: return UCurseSpell::StaticClass();
    default:
        return nullptr;
    }
}
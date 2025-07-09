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

    UUSpellBase* Spell = NewObject<UUSpellBase>(Character, SpellClass);
    if (Spell && Spell->CanActivate(Character))
    {

        // 마법 시전 몽타주 재생
        if (Character)
        {
            UE_LOG(LogTemp, Warning, TEXT("Trying to play montage"));
            Character->PlayMagicMontage();
        }

        // 스크롤용 특별 처리 - 클래스 체크 없이 직접 서버 RPC 호출
        FVector TargetLocation = FVector::ZeroVector;
        FRotator TargetRotation = Character->GetControlRotation();

        if (Data.SkillIndex == 0) // Fireball
        {
            TargetLocation = Character->GetActorLocation() + Character->GetActorForwardVector() * 200.0f + FVector(0, 0, 50.0f);
        }
        else if (Data.SkillIndex == 1) // Heal - 추가!
        {
            TargetLocation = Character->GetActorLocation(); // 힐은 자기 위치에
        }
        else if (Data.SkillIndex == 2) // Curse
        {
            // Curse 타겟 계산
            FVector Start = Character->GetActorLocation() + Character->GetActorForwardVector() * 100.0f;
            FVector End = Start + Character->GetActorForwardVector() * 1000.0f;
            TargetLocation = End;
        }

        // 직접 ServerCastSpell 호출 (클래스 체크 우회)
        if (Character->HasAuthority())
        {
            // 서버라면 직접 실행
            Character->ExecuteSpellOnServer(Data.SkillIndex, TargetLocation, TargetRotation);
            Character->MulticastPlaySpellCastAnimation();
        }
        else
        {
            // 클라이언트라면 서버 RPC
            Character->ServerCastSpell(Data.SkillIndex, TargetLocation, TargetRotation);
        }

        //Spell->ActivateSpell(Character);
        
        // 스펠 인덱스를 가져와서 캐릭터의 멀티플레이어 캐스팅 함수 호출
        //Character->TryCastSpellMultiplayer(Data.SkillIndex);
        
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
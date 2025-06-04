// Fill out your copyright notice in the Description page of Project Settings.


#include "USpellBase.h"
#include "MyDCharacter.h"

void UUSpellBase::ActivateSpell(AMyDCharacter* Caster)
{
    // 기본 구현: 아무 일도 안 함 (서브클래스에서 오버라이드해야 함)

    

    UE_LOG(LogTemp, Warning, TEXT("USpellBase::ActivateSpell() called - override this in child class!"));
}

bool UUSpellBase::CanActivate(AMyDCharacter* Caster) const
{
    if (!Caster) return false;
    return Caster->Knowledge >= ManaCost && Caster->Stamina >= StaminaCost;
}

// Fill out your copyright notice in the Description page of Project Settings.


#include "USpellBase.h"
#include "MyDCharacter.h"

void UUSpellBase::ActivateSpell(AMyDCharacter* Caster)
{
    // �⺻ ����: �ƹ� �ϵ� �� �� (����Ŭ�������� �������̵��ؾ� ��)

    

    UE_LOG(LogTemp, Warning, TEXT("USpellBase::ActivateSpell() called - override this in child class!"));
}

bool UUSpellBase::CanActivate(AMyDCharacter* Caster) const
{
    if (!Caster) return false;
    return Caster->Knowledge >= ManaCost && Caster->Stamina >= StaminaCost;
}

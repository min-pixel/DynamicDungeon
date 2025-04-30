// Fill out your copyright notice in the Description page of Project Settings.


#include "DynamicDungeonInstance.h"

UDynamicDungeonInstance ::UDynamicDungeonInstance
()
{
    // 기본값 설정
    bCanInteract = false;
}

void UDynamicDungeonInstance::SetCanInteract(bool NewState)
{
    bCanInteract = NewState;
}

void UDynamicDungeonInstance::InitializeCharacterData(EPlayerClass SelectedClass)
{
    CurrentCharacterData.PlayerClass = SelectedClass;
    CurrentCharacterData.Level = 1;
    CurrentCharacterData.PlayerName = TEXT("DefaultName"); // 나중에 입력받게 해도 됨

    switch (SelectedClass)
    {
    case EPlayerClass::Warrior:
        CurrentCharacterData.MaxHealth = 150.f;
        CurrentCharacterData.MaxStamina = 100.f;
        CurrentCharacterData.MaxKnowledge = 50.f;
        break;

    case EPlayerClass::Rogue:
        CurrentCharacterData.MaxHealth = 100.f;
        CurrentCharacterData.MaxStamina = 150.f;
        CurrentCharacterData.MaxKnowledge = 50.f;
        break;

    case EPlayerClass::Mage:
        CurrentCharacterData.MaxHealth = 10.f;
        CurrentCharacterData.MaxStamina = 10.f;
        CurrentCharacterData.MaxKnowledge = 200.f;
        break;
    }

    // 빈 인벤토리 / 장비 초기화
    CurrentCharacterData.InventoryItems.Empty();
    CurrentCharacterData.EquippedItems.Empty();
}
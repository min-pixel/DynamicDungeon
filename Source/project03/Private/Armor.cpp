// Fill out your copyright notice in the Description page of Project Settings.

#include "Armor.h"
#include "MyDCharacter.h"


AArmor::AArmor()
{
    ItemType = EItemType::Armor;  // ¸ðµç °©¿Ê °øÅë
}

void AArmor::ApplyArmorStats(AMyDCharacter* Character)
{
	if (Character)
	{
		Character->MaxHealth += BonusHealth;
		Character->Health = FMath::Clamp(Character->Health + BonusHealth, 0.0f, Character->MaxHealth);
		//Character->UpdateHUD();
	}
}

void AArmor::RemoveArmorStats(AMyDCharacter* Character)
{
	if (Character)
	{
		Character->MaxHealth -= BonusHealth;
		Character->Health = FMath::Clamp(Character->Health, 0.0f, Character->MaxHealth);
		//Character->UpdateHUD();
	}
}
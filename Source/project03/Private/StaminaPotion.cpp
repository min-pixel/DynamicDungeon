// Fill out your copyright notice in the Description page of Project Settings.


#include "StaminaPotion.h"

AStaminaPotion::AStaminaPotion()
{
	ItemName = TEXT("StaminaPotion");
	ItemType = EItemType::Potion;
	PotionEffect = EPotionEffectType::Stamina;
	ItemData.PotionEffect = EPotionEffectType::Stamina;
	ItemData.Count = 1;
	Price = 40;
	StaminaAmount = 40;

	static ConstructorHelpers::FObjectFinder<UTexture2D> IconTexture(TEXT("/Game/BP/Icon/elixir.elixir"));
	if (IconTexture.Succeeded())
	{
		ItemIcon = IconTexture.Object;
	}
}
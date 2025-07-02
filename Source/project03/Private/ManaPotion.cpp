// Fill out your copyright notice in the Description page of Project Settings.


#include "ManaPotion.h"

AManaPotion::AManaPotion()
{
	ItemName = TEXT("ManaPotion");
	ItemType = EItemType::Potion;
	PotionEffect = EPotionEffectType::Mana;
	ItemData.PotionEffect = EPotionEffectType::Mana;
	ItemData.Count = 1;
	Price = 60;
	ManaAmount = 40;

	static ConstructorHelpers::FObjectFinder<UTexture2D> IconTexture(TEXT("/Game/BP/Icon/flask.flask"));
	if (IconTexture.Succeeded())
	{
		ItemIcon = IconTexture.Object;
	}
}
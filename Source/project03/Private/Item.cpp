// Fill out your copyright notice in the Description page of Project Settings.


#include "Item.h"
#include "Armor.h"
#include "ScrollItem.h"
#include "Potion.h"
#include "ManaPotion.h"
#include "StaminaPotion.h"
#include "Weapon.h"



// Sets default values
AItem::AItem()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

}

// Called when the game starts or when spawned
void AItem::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AItem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void AItem::SetDefaultIcon()
{
    
    UTexture2D* LoadedTexture = LoadObject<UTexture2D>(nullptr, TEXT("/Engine/EngineResources/DefaultTexture.DefaultTexture"));
    if (LoadedTexture)
    {
        ItemIcon = LoadedTexture;
    }
}

FItemData AItem::ToItemData() const
{
    FItemData Data;
    Data.ItemClass = GetClass();
    Data.ItemName = ItemName;
    Data.ItemIcon = ItemIcon;
    Data.ItemType = ItemType;
    Data.bIsStackable = bIsStackable;
    Data.MaxStack = MaxStack;
    Data.PotionEffect = PotionEffect;
    Data.Count = 1; // 기본 수량
    Data.Price = Price;

    if (const APotion* HealthPotion = Cast<APotion>(this))
    {
        Data.PotionEffect = EPotionEffectType::Health;
        UE_LOG(LogTemp, Warning, TEXT("ToItemData: HealthPotion detected, setting effect to Health"));
    }
    else if (const AManaPotion* ManaPotion = Cast<AManaPotion>(this))
    {
        Data.PotionEffect = EPotionEffectType::Mana;
        UE_LOG(LogTemp, Warning, TEXT("ToItemData: ManaPotion detected, setting effect to Mana"));
    }
    else if (const AStaminaPotion* StaminaPotion = Cast<AStaminaPotion>(this))
    {
        Data.PotionEffect = EPotionEffectType::Stamina;
        UE_LOG(LogTemp, Warning, TEXT("ToItemData: StaminaPotion detected, setting effect to Stamina"));
    }
    else
    {
        Data.PotionEffect = PotionEffect;  // 기본값 사용
    }


    if (const AArmor* Armor = Cast<AArmor>(this))
    {
        Data.Grade = static_cast<uint8>(Armor->ArmorGrade);
    }

    // 스크롤일 경우 스킬 인덱스 복사
    if (const AScrollItem* Scroll = Cast<AScrollItem>(this))
    {
        Data.SkillIndex = FMath::RandRange(0, 2);

        //Scroll->InitFromData(Data);
    }
    return Data;
}

void AItem::Use_Implementation(AMyDCharacter* Character)
{
    // 기본 아이템은 아무 행동도 안 함
    UE_LOG(LogTemp, Log, TEXT("AItem::Use_Implementation() called, but no effect."));
}
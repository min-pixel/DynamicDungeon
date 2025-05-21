// Fill out your copyright notice in the Description page of Project Settings.


#include "Item.h"


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
    return Data;
}
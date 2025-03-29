// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryComponent.h"

// Sets default values for this component's properties
UInventoryComponent::UInventoryComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
    InventoryItems.SetNum(Capacity); // Capacity만큼 슬롯 확보
	// ...
}


// Called when the game starts
void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
    InventoryItems.Init(nullptr, Capacity);
	// ...
	
}


// Called every frame
void UInventoryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}


bool UInventoryComponent::TryAddItem(AItem* NewItem)
{
    if (!NewItem) return false;

    for (int32 i = 0; i < InventoryItems.Num(); ++i)
    {
        if (!InventoryItems[i])
        {
            InventoryItems[i] = NewItem;
            return true;
        }
    }

    return false; // 공간 없음
}

bool UInventoryComponent::RemoveItemAt(int32 Index)
{
    if (InventoryItems.IsValidIndex(Index) && InventoryItems[Index])
    {
        InventoryItems[Index] = nullptr;
        return true;
    }
    return false;
}


// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryComponent.h"

// Sets default values for this component's properties
UInventoryComponent::UInventoryComponent()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
    //InventoryItems.SetNum(Capacity); // Capacity만큼 슬롯 확보
    InventoryItemsStruct.SetNum(Capacity);
	// ...

    HotkeyItems.SetNum(5);

}


// Called when the game starts
void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();
    //InventoryItems.Init(nullptr, Capacity);
	// ...
	
}


// Called every frame
void UInventoryComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}


//bool UInventoryComponent::TryAddItem(AItem* NewItem)
//{
//    if (!NewItem) return false;
//
//    for (int32 i = 0; i < InventoryItems.Num(); ++i)
//    {
//        if (!InventoryItems[i])
//        {
//            InventoryItems[i] = NewItem;
//            //NewItem->Destroy();
//            return true;
//        }
//    }
//
//    return false; // 공간 없음
//}
//
//bool UInventoryComponent::RemoveItemAt(int32 Index)
//{
//    if (InventoryItems.IsValidIndex(Index) && InventoryItems[Index])
//    {
//        InventoryItems[Index] = nullptr;
//        return true;
//    }
//    return false;
//}

bool UInventoryComponent::TryAddItemByClass(TSubclassOf<AItem> ItemClass)
{
    if (!ItemClass) return false;

    AItem* DefaultItem = ItemClass->GetDefaultObject<AItem>();
    if (!DefaultItem) return false;

    FItemData NewData = DefaultItem->ToItemData();

    for (int32 i = 0; i < InventoryItemsStruct.Num(); ++i)
    {
        if (InventoryItemsStruct[i].ItemClass == nullptr)
        {
            InventoryItemsStruct[i] = NewData;
            return true;
        }
    }

    return false;
}

bool UInventoryComponent::RemoveItemAtStruct(int32 Index)
{
    if (InventoryItemsStruct.IsValidIndex(Index))
    {
        InventoryItemsStruct[Index] = FItemData(); // 빈 구조체로 초기화
        return true;
    }
    return false;
}
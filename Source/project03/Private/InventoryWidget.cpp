// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryWidget.h"
#include "SlotWidget.h"
#include "Item.h"
#include "InventoryComponent.h"
#include "Components/UniformGridPanel.h"
#include "Components/WrapBox.h"
#include "Blueprint/DragDropOperation.h"
#include "Components/UniformGridSlot.h"

//void UInventoryWidget::RefreshInventory()
//{
//    if (!ItemContainer || !InventoryRef)
//    {
//        UE_LOG(LogTemp, Error, TEXT("ItemContainer or InventoryRef is NULL!"));
//        return;
//    }
//
//    ItemContainer->ClearChildren();
//
//    const int32 SlotCount = InventoryRef->InventoryItems.Num();
//    UE_LOG(LogTemp, Warning, TEXT("Refreshing Inventory: %d slots"), SlotCount);
//
//    for (int32 Index = 0; Index < SlotCount; ++Index)
//    {
//        AItem* Item = InventoryRef->InventoryItems[Index];
//
//        USlotWidget* NewSlot = CreateWidget<USlotWidget>(this, SlotWidgetClass);
//        if (NewSlot)
//        {
//            NewSlot->SetItem(Item);  // nullptr인 경우도 허용
//            NewSlot->InventoryOwner = this;
//            NewSlot->SlotIndex = Index;
//            ItemContainer->AddChild(NewSlot);
//
//            if (Item)
//            {
//                UE_LOG(LogTemp, Log, TEXT("Slot %d: %s"), Index, *Item->ItemName);
//            }
//            else
//            {
//                UE_LOG(LogTemp, Log, TEXT("Slot %d is empty"), Index);
//            }
//        }
//        else
//        {
//            UE_LOG(LogTemp, Error, TEXT("Failed to create SlotWidget at index %d"), Index);
//        }
//    }
//}

//bool UInventoryWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
//{
//    USlotWidget* DroppedSlot = Cast<USlotWidget>(InOperation->Payload);
//    if (!DroppedSlot || !InventoryRef) return false;
//
//    AItem* DroppedItem = DroppedSlot->GetStoredItem();
//    int32 DropIndex = InventoryRef->InventoryItems.Find(DroppedItem);
//
//    if (DropIndex != INDEX_NONE)
//    {
//        // 월드에 아이템 드롭
//        DroppedItem->SetActorLocation(GetOwningPlayerPawn()->GetActorLocation() + FVector(100, 0, 0));
//        DroppedItem->SetActorHiddenInGame(false);
//        DroppedItem->SetActorEnableCollision(true);
//
//        InventoryRef->InventoryItems[DropIndex] = nullptr;
//
//        RefreshInventory();
//
//        UE_LOG(LogTemp, Warning, TEXT("Item dropped into world: %s"), *DroppedItem->GetName());
//    }
//
//    return true;
//}

void UInventoryWidget::RefreshInventoryStruct()
{
    if (!ItemContainer || !InventoryRef) return;

    ItemContainer->ClearChildren();

    const int32 SlotCount = InventoryRef->InventoryItemsStruct.Num();
    for (int32 Index = 0; Index < SlotCount; ++Index)
    {
        const FItemData& ItemData = InventoryRef->InventoryItemsStruct[Index];

        USlotWidget* NewSlot = CreateWidget<USlotWidget>(this, SlotWidgetClass);
        if (NewSlot)
        {
            NewSlot->SetItemData(ItemData);
            NewSlot->InventoryOwner = this;
            NewSlot->SlotIndex = Index;
            ItemContainer->AddChild(NewSlot);

            UE_LOG(LogTemp, Warning, TEXT("Refreshing Inventory (Struct): %d slots"), SlotCount);
            UE_LOG(LogTemp, Warning, TEXT("Slot %d: %s"), Index, *ItemData.ItemName);

        }
    }
}
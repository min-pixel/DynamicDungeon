// Fill out your copyright notice in the Description page of Project Settings.


#include "InventoryWidget.h"
#include "SlotWidget.h"
#include "InventoryComponent.h"
#include "Components/UniformGridPanel.h"
#include "Components/WrapBox.h"
#include "Components/UniformGridSlot.h"

void UInventoryWidget::RefreshInventory()
{
    if (!ItemContainer || !InventoryRef)
    {
        UE_LOG(LogTemp, Error, TEXT("ItemContainer or InventoryRef is NULL!"));
        return;
    }

    ItemContainer->ClearChildren();

    for (AItem* Item : InventoryRef->InventoryItems)
    {
        if (Item)
        {
            UE_LOG(LogTemp, Log, TEXT("Found item in inventory: %s"), *Item->GetName());

            USlotWidget* NewSlot = CreateWidget<USlotWidget>(this, SlotWidgetClass);
            if (NewSlot)
            {
                NewSlot->SetItem(Item);
                ItemContainer->AddChild(NewSlot);
            }
        }
    }
}
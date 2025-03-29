// Fill out your copyright notice in the Description page of Project Settings.


#include "SlotWidget.h"
#include "Components/Image.h"
#include "Item.h"

void USlotWidget::SetItem(AItem* NewItem)
{
    StoredItem = NewItem;

    if (ItemIcon && StoredItem)
    {
        ItemIcon->SetBrushFromTexture(StoredItem->ItemIcon);
        UE_LOG(LogTemp, Log, TEXT("Set item icon for: %s"), *StoredItem->ItemName);
    }
}

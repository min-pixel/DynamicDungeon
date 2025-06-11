// Fill out your copyright notice in the Description page of Project Settings.


#include "ShopWidget.h"
#include "InventoryComponent.h"
#include "SlotWidget.h"
#include "Components/WrapBox.h"
#include "Blueprint/UserWidget.h"
#include "LobbyWidget.h"

void UShopWidget::PopulateShopItems()
{
    if (!ShopItemContainer) return;

    ShopItemContainer->ClearChildren();

    for (int32 i = 0; i < ShopItemList.Num(); ++i)
    {
        USlotWidget* NewSlot = CreateWidget<USlotWidget>(this, SlotWidgetClass);
        if (NewSlot)
        {
            NewSlot->SetItemData(ShopItemList[i]);
            NewSlot->bIsShopSlot = true; 
            NewSlot->bIsChestInventory = true;
            NewSlot->InventoryOwner = nullptr;
            NewSlot->SlotIndex = i;
            ShopItemContainer->AddChild(NewSlot);
        }
    }
}

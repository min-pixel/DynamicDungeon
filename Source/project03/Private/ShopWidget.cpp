// Fill out your copyright notice in the Description page of Project Settings.


#include "ShopWidget.h"
#include "InventoryComponent.h"
#include "SlotWidget.h"
#include "Components/WrapBox.h"
#include "Blueprint/UserWidget.h"
#include "LobbyWidget.h"

void UShopWidget::PopulateShopItems()
{
    if (!ShopItemContainer || !SlotWidgetClass) return;

    ShopItemContainer->ClearChildren();

    const int32 NumDefaultItems = ShopItemList.Num();
    const int32 NumExtraSlots = 10; // ¿©À¯ ½½·Ô ¼ö
    const int32 TotalSlots = NumDefaultItems + NumExtraSlots;

    for (int32 i = 0; i < TotalSlots; ++i)
    {
        USlotWidget* NewSlot = CreateWidget<USlotWidget>(this, SlotWidgetClass);
        if (NewSlot)
        {
            if (i < NumDefaultItems)
                NewSlot->SetItemData(ShopItemList[i]);
            else
                NewSlot->SetItemData(FItemData()); // ºó ½½·Ô

            NewSlot->bIsShopSlot = true;
            NewSlot->bIsChestInventory = true;
            NewSlot->InventoryOwner = nullptr;
            NewSlot->SlotIndex = i;
            ShopItemContainer->AddChild(NewSlot);
        }
    }
}

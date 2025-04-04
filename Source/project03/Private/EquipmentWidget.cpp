// Fill out your copyright notice in the Description page of Project Settings.


#include "EquipmentWidget.h"




void UEquipmentWidget::SetSlot(int32 Index, const FItemData& ItemData)
{
    if (EquipmentSlots.IsValidIndex(Index))
    {
        EquipmentSlots[Index] = ItemData;
    }
}

void UEquipmentWidget::ClearSlot(int32 Index)
{
    if (EquipmentSlots.IsValidIndex(Index))
    {
        EquipmentSlots[Index] = FItemData();
    }
}

void UEquipmentWidget::SwapSlots(int32 From, int32 To)
{
    if (EquipmentSlots.IsValidIndex(From) && EquipmentSlots.IsValidIndex(To))
    {
        EquipmentSlots.Swap(From, To);
    }
}

void UEquipmentWidget::RefreshEquipmentSlots()
{
    EquipmentSlotContainer->ClearChildren();
    for (int32 i = 0; i < EquipmentSlots.Num(); ++i)
    {
        USlotWidget* NewSlot = CreateWidget<USlotWidget>(this, SlotWidgetClass);
        if (NewSlot)
        {
            NewSlot->SetItemData(EquipmentSlots[i]);
            NewSlot->SlotIndex = i;
            NewSlot->bIsEquipmentSlot = true;
            NewSlot->EquipmentOwner = this;
            EquipmentSlotContainer->AddChild(NewSlot);
        }
    }
}
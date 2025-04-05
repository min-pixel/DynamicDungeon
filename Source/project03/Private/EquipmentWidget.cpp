// Fill out your copyright notice in the Description page of Project Settings.


#include "EquipmentWidget.h"
#include "MyDCharacter.h"




void UEquipmentWidget::SetSlot(int32 Index, const FItemData& ItemData)
{
    if (EquipmentSlots.IsValidIndex(Index))
    {
        EquipmentSlots[Index] = ItemData;
    }

    if (Index == EQUIP_SLOT_WEAPON && ItemData.ItemType == EItemType::Weapon)
    {
        if (AMyDCharacter* Char = Cast<AMyDCharacter>(GetOwningPlayerPawn()))
        {
            UE_LOG(LogTemp, Error, TEXT("GGGGGGGG"));
            Char->EquipWeaponFromClass(ItemData.ItemClass);
        }
    }

}

void UEquipmentWidget::ClearSlot(int32 Index)
{
    if (EquipmentSlots.IsValidIndex(Index))
    {

        // 무기슬롯이라면 무기 해제
        if (Index == EQUIP_SLOT_WEAPON)
        {
            if (AMyDCharacter* Char = Cast<AMyDCharacter>(GetOwningPlayerPawn()))
            {
                Char->UnequipWeapon();
            }
        }


        EquipmentSlots[Index] = FItemData();
        if (EquipmentSlotContainer && EquipmentSlotContainer->GetChildrenCount() > Index)
        {
            USlotWidget* TargetSlot = Cast<USlotWidget>(EquipmentSlotContainer->GetChildAt(Index));
            if (TargetSlot)
            {
                TargetSlot->SetItemData(FItemData());
            }
        }
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
    EquipmentSlots.SetNum(4);
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
            NewSlot->InventoryOwner = this->InventoryOwner;
            EquipmentSlotContainer->AddChild(NewSlot);
        }
    }
}
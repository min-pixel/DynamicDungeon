// Fill out your copyright notice in the Description page of Project Settings.


#include "EquipmentWidget.h"
#include "MyDCharacter.h"




void UEquipmentWidget::SetSlot(int32 Index, const FItemData& ItemData)
{
    if (!EquipmentSlots.IsValidIndex(Index))
    {
        return;
    }

    EquipmentSlots[Index] = ItemData;

   // ĳ���Ͱ� ���� ���� ���� ���� �õ�
    APawn* OwnerPawn = GetOwningPlayerPawn();
    if (OwnerPawn)
    {
        if (Index == EQUIP_SLOT_WEAPON && ItemData.ItemType == EItemType::Weapon)
        {
            if (AMyDCharacter* Char = Cast<AMyDCharacter>(OwnerPawn))
            {
                Char->EquipWeaponFromClass(ItemData.ItemClass);
                UE_LOG(LogTemp, Log, TEXT("Weapon Equipped in Game"));
            }
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("No owning player pawn (probably Lobby)"));
    }
    RefreshEquipmentSlots();
}

void UEquipmentWidget::ClearSlot(int32 Index)
{
    if (EquipmentSlots.IsValidIndex(Index))
    {

        // ���⽽���̶�� ���� ����
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
    EquipmentSlots.SetNum(9);
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

            // ��Ű �����̸� ǥ�ÿ� �÷��� ����
            if (i >= 4)
            {
                NewSlot->bIsHotkeySlot = true;
            }

            EquipmentSlotContainer->AddChild(NewSlot);
        }
    }
}

TArray<FItemData> UEquipmentWidget::GetAllEquipmentData() const
{
    return EquipmentSlots;
}

void UEquipmentWidget::RestoreEquipmentFromData(const TArray<FItemData>& SavedData)
{
    EquipmentSlots = SavedData;
    RefreshEquipmentSlots();
}
// Fill out your copyright notice in the Description page of Project Settings.


#include "EquipmentWidget.h"
#include "UCharacterHUDWidget.h"
#include "MyDCharacter.h"




void UEquipmentWidget::SetSlot(int32 Index, const FItemData& ItemData)
{
    if (!EquipmentSlots.IsValidIndex(Index))
    {
        return;
    }

    EquipmentSlots[Index] = ItemData;

    const int32 HotkeyStartIndex = 4;
    const int32 HotkeyEndIndex = 8;

    if (Index >= HotkeyStartIndex && Index <= HotkeyEndIndex)
    {
        APawn* OwnerPawn = GetOwningPlayerPawn();
        if (AMyDCharacter* Char = Cast<AMyDCharacter>(OwnerPawn))
        {
            int32 HotkeyIndex = Index - HotkeyStartIndex;
            if (Char->HUDWidget)
            {
                Char->HUDWidget->UpdateHotkeySlot(HotkeyIndex, ItemData);
            }
        }
    }


   // 캐릭터가 있을 때만 무기 장착 시도
    APawn* OwnerPawn = GetOwningPlayerPawn();
    if (AMyDCharacter* Char = Cast<AMyDCharacter>(OwnerPawn))
    {
        if (Index == EQUIP_SLOT_WEAPON && ItemData.ItemType == EItemType::Weapon)
        {
            Char->EquipWeaponFromClass(ItemData.ItemClass, static_cast<EWeaponGrade>(ItemData.Grade));
            UE_LOG(LogTemp, Log, TEXT("Weapon Equipped in Game"));
        }
        else if (ItemData.ItemType == EItemType::Armor)
        {
            Char->EquipArmorFromClass(Index, ItemData.ItemClass, ItemData.Grade);
            UE_LOG(LogTemp, Log, TEXT("Armor Equipped at slot %d"), Index);
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
        AMyDCharacter* Char = Cast<AMyDCharacter>(GetOwningPlayerPawn());
        if (Char)
        {
            if (Index == EQUIP_SLOT_WEAPON)
            {
                Char->UnequipWeapon();
            }
            else if (EquipmentSlots[Index].ItemType == EItemType::Armor)
            {
                Char->UnequipArmorAtSlot(Index);
                UE_LOG(LogTemp, Log, TEXT("Armor unequipped at slot %d"), Index);
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

            // 핫키 슬롯이면 표시용 플래그 설정
            if (i >= 4)
            {
                NewSlot->bIsHotkeySlot = true;

                const FItemData& Data = EquipmentSlots[i];
                if (Data.ItemClass)
                {
                    UE_LOG(LogTemp, Log, TEXT("Hotkey Slot %d: %s (%s)"),
                        i,
                        *Data.ItemName,
                        *Data.ItemClass->GetName());
                }
                else
                {
                    UE_LOG(LogTemp, Log, TEXT("Hotkey Slot %d: [Empty]"), i);
                }

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
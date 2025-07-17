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

    // ��Ű ���� ������Ʈ (���� Ŭ���̾�Ʈ������)
    const int32 HotkeyStartIndex = 4;
    const int32 HotkeyEndIndex = 8;

    if (Index >= HotkeyStartIndex && Index <= HotkeyEndIndex)
    {
        APawn* OwnerPawn = GetOwningPlayerPawn();
        if (AMyDCharacter* Char = Cast<AMyDCharacter>(OwnerPawn))
        {
            // ���� Ŭ���̾�Ʈ������ ��Ű UI ������Ʈ
            if (Char->IsLocallyControlled() && Char->HUDWidget)
            {
                int32 HotkeyIndex = Index - HotkeyStartIndex;
                Char->HUDWidget->UpdateHotkeySlot(HotkeyIndex, ItemData);
                UE_LOG(LogTemp, Log, TEXT("Updated hotkey slot %d with item: %s"),
                    HotkeyIndex, *ItemData.ItemName);
            }
        }
    }

    // ĳ���Ͱ� ���� ���� ���� ���� �õ�
    APawn* OwnerPawn = GetOwningPlayerPawn();
    if (AMyDCharacter* Char = Cast<AMyDCharacter>(OwnerPawn))
    {
        if (Index == EQUIP_SLOT_WEAPON && ItemData.ItemType == EItemType::Weapon)
        {
            Char->ServerRequestEquipWeapon(ItemData);
            UE_LOG(LogTemp, Log, TEXT("Weapon Equipped in Game"));
        }
        else if (ItemData.ItemType == EItemType::Armor)
        {
            Char->ServerRequestEquipArmor(ItemData, Index);
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
                Char->ServerRequestUnequipWeapon();
            }
            else if (EquipmentSlots[Index].ItemType == EItemType::Armor)
            {
                // ���� ��û ���� ��� �ð��� ���� (�ɼ�)
                if (Char->IsLocallyControlled())
                {
                    Char->UnequipArmorAtSlot(Index);
                }
                Char->ServerRequestUnequipArmor(Index);
                UE_LOG(LogTemp, Log, TEXT("Armor unequipped at slot %d"), Index);
            }
        }

        // ��Ű ���� Ŭ���� (���� Ŭ���̾�Ʈ������)
        const int32 HotkeyStartIndex = 4;
        const int32 HotkeyEndIndex = 8;

        if (Index >= HotkeyStartIndex && Index <= HotkeyEndIndex && Char)
        {
            if (Char->IsLocallyControlled() && Char->HUDWidget)
            {
                int32 HotkeyIndex = Index - HotkeyStartIndex;
                FItemData EmptyItem; // �� ������
                Char->HUDWidget->UpdateHotkeySlot(HotkeyIndex, EmptyItem);
                UE_LOG(LogTemp, Log, TEXT("Cleared hotkey slot %d"), HotkeyIndex);
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

        // ��Ű ������ ���õ� ��� ��Ű UI�� ������Ʈ
        const int32 HotkeyStartIndex = 4;
        const int32 HotkeyEndIndex = 8;

        AMyDCharacter* Char = Cast<AMyDCharacter>(GetOwningPlayerPawn());
        if (Char && Char->IsLocallyControlled() && Char->HUDWidget)
        {
            if ((From >= HotkeyStartIndex && From <= HotkeyEndIndex) ||
                (To >= HotkeyStartIndex && To <= HotkeyEndIndex))
            {
                // ��ü ��Ű ���� ���ΰ�ħ
                for (int32 i = HotkeyStartIndex; i <= HotkeyEndIndex; ++i)
                {
                    int32 HotkeyIndex = i - HotkeyStartIndex;
                    Char->HUDWidget->UpdateHotkeySlot(HotkeyIndex, EquipmentSlots[i]);
                }
            }
        }
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

    // RefreshEquipmentSlots �Ŀ� ��Ű UI�� ����ȭ (���� Ŭ���̾�Ʈ������)
    AMyDCharacter* Char = Cast<AMyDCharacter>(GetOwningPlayerPawn());
    if (Char && Char->IsLocallyControlled() && Char->HUDWidget)
    {
        const int32 HotkeyStartIndex = 4;
        const int32 HotkeyEndIndex = 8;

        for (int32 i = HotkeyStartIndex; i <= HotkeyEndIndex; ++i)
        {
            int32 HotkeyIndex = i - HotkeyStartIndex;
            Char->HUDWidget->UpdateHotkeySlot(HotkeyIndex, EquipmentSlots[i]);
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

    // ������ ���� �� ��Ű UI�� ����ȭ (���� Ŭ���̾�Ʈ������)
    AMyDCharacter* Char = Cast<AMyDCharacter>(GetOwningPlayerPawn());
    if (Char && Char->IsLocallyControlled() && Char->HUDWidget)
    {
        const int32 HotkeyStartIndex = 4;
        const int32 HotkeyEndIndex = 8;

        for (int32 i = HotkeyStartIndex; i <= HotkeyEndIndex; ++i)
        {
            if (EquipmentSlots.IsValidIndex(i))
            {
                int32 HotkeyIndex = i - HotkeyStartIndex;
                Char->HUDWidget->UpdateHotkeySlot(HotkeyIndex, EquipmentSlots[i]);
            }
        }
        UE_LOG(LogTemp, Log, TEXT("Hotkey UI synchronized after data restoration"));
    }
}
// Fill out your copyright notice in the Description page of Project Settings.


#include "SlotWidget.h"
#include "Components/Image.h"
#include "InventoryWidget.h"
#include "InventoryComponent.h"
#include "Blueprint/DragDropOperation.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "EquipmentWidget.h"
#include "Item.h"
#include "MyDCharacter.h"
#include "GoldWidget.h"
#include "LobbyWidget.h"
#include "DynamicDungeonInstance.h"
#include "ItemTooltipWidget.h"

//void USlotWidget::SetItem(AItem* InItem)
//{
//    StoredItem = InItem;
//
//    if (!ItemIcon) return;
//
//    if (StoredItem && StoredItem->ItemIcon)
//    {
//        ItemIcon->SetBrushFromTexture(StoredItem->ItemIcon);
//        ItemIcon->SetColorAndOpacity(FLinearColor::White); // ������ ���̰�
//    }
//    else
//    {
//        // �� ���Կ� �⺻ ȸ�� ĥ�ϱ�
//        FSlateBrush EmptyBrush;
//        EmptyBrush.TintColor = FLinearColor(0.2f, 0.2f, 0.2f, 0.8f); // ȸ�� ������
//        ItemIcon->SetBrush(EmptyBrush);
//    }
//}
//
FReply USlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
    {
        return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;

    }

    /*if (bIsShopSlot && InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
    {
        if (InventoryOwner && InventoryOwner->InventoryRef)
        {
            InventoryOwner->InventoryRef->TryAddItemByClassWithGrade(StoredData.ItemClass, StoredData.Grade);
            UE_LOG(LogTemp, Log, TEXT("Shop: Purchased %s"), *StoredData.ItemName);
        }
        return FReply::Handled();
    }*/

    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

//void USlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
//{
//    if (!StoredItem) return;
//
//    UDragDropOperation* DragOp = NewObject<UDragDropOperation>();
//    DragOp->Payload = this;
//    DragOp->DefaultDragVisual = this;  // �ӽ�: ���� ���� ��ü�� ����ٴ�
//    DragOp->Pivot = EDragPivot::MouseDown;
//
//    OutOperation = DragOp;
//
//    SetVisibility(ESlateVisibility::Hidden);  // �ð������� ���� (�巡�� �� ǥ�� ����)
//}


//void USlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
//{
//    if (!StoredItem) return;
//
//    UDragDropOperation* DragOp = NewObject<UDragDropOperation>();
//    DragOp->Payload = this; // ���߿� ����� �� �ֵ��� ���� ���� ���� ����
//    //DragOp->DefaultDragVisual = this; // ���� ���� �״�� ���̰�
//
//     // ���ο� �ð��� ������ ����
//    USlotWidget* DragVisual = CreateWidget<USlotWidget>(GetWorld(), GetClass());
//    DragVisual->SetItem(StoredItem);  // �ð������� �����ϰ�
//    DragVisual->SetVisibility(ESlateVisibility::SelfHitTestInvisible); // Ŭ�� ���� �ʰ�
//
//    DragOp->DefaultDragVisual = DragVisual;
//    DragOp->Pivot = EDragPivot::MouseDown;
//
//    OutOperation = DragOp;
//}
//
//bool USlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
//{
//    if (!InOperation || !InventoryOwner) return false;
//
//    USlotWidget* SourceSlot = Cast<USlotWidget>(InOperation->Payload);
//    if (!SourceSlot || SourceSlot == this) return false;
//
//    // �ε��� ��� (���߿� ��Ȯ�� ���� �ε��� �ý����� �ִٸ� �װɷ� ��ü)
//    int32 FromIndex = SourceSlot->SlotIndex;
//    int32 ToIndex = this->SlotIndex;
//
//    if (!InventoryOwner->InventoryRef) return false;
//
//
//    // ����
//    InventoryOwner->InventoryRef->InventoryItems.Swap(FromIndex, ToIndex);
//    InventoryOwner->RefreshInventory();
//
//    return true;
//}

void USlotWidget::SetItemData(const FItemData& NewData)
{
    StoredData = NewData;


    if (StoredData.ItemClass)
    {
        UE_LOG(LogTemp, Warning, TEXT("SetItemData - Class: %s, Icon: %s, Name: %s"),
            *StoredData.ItemClass->GetName(),
            StoredData.ItemIcon ? *StoredData.ItemIcon->GetName() : TEXT("NULL"),
            *StoredData.ItemName);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("SetItemData - ItemClass is NULL"));
    }


    if (!ItemIcon) return;

    if (StoredData.ItemIcon)
    {
        ItemIcon->SetBrushFromTexture(StoredData.ItemIcon);
        ItemIcon->SetColorAndOpacity(FLinearColor::White);

    }
    else
    {
        FSlateBrush EmptyBrush;
        EmptyBrush.TintColor = FLinearColor(0.2f, 0.2f, 0.2f, 0.8f);
        ItemIcon->SetBrush(EmptyBrush);
    }


    // ���� ���� ����
    if (TooltipWidgetClass && StoredData.ItemClass)
    {
        UItemTooltipWidget* TooltipWidget = CreateWidget<UItemTooltipWidget>(
            GetWorld(), TooltipWidgetClass
        );
        if (TooltipWidget)
        {
            TooltipWidget->InitWithItemData(StoredData);
            SetToolTip(TooltipWidget);
            UE_LOG(LogTemp, Warning, TEXT("[SlotWidget] SetToolTip() called with widget: %s"), *TooltipWidget->GetName());
        }
    }
    else
    {
        SetToolTip(nullptr);
        UE_LOG(LogTemp, Warning, TEXT("[SlotWidget] Empty slot or TooltipWidgetClass is NULL, tooltip removed"));
    }

}

void USlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
    if (StoredData.ItemClass == nullptr) return;

    UDragDropOperation* DragOp = NewObject<UDragDropOperation>();
    DragOp->Payload = this;

    USlotWidget* DragVisual = CreateWidget<USlotWidget>(GetWorld(), GetClass());
    DragVisual->SetItemData(StoredData);

    DragVisual->SlotIndex = SlotIndex;
    DragVisual->bIsEquipmentSlot = bIsEquipmentSlot;
    DragVisual->InventoryOwner = InventoryOwner;
    DragVisual->EquipmentOwner = EquipmentOwner;

    DragVisual->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

    DragOp->DefaultDragVisual = DragVisual;
    DragOp->Pivot = EDragPivot::MouseDown;

    OutOperation = DragOp;
}

bool USlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    USlotWidget* SourceSlot = Cast<USlotWidget>(InOperation->Payload);
    if (!SourceSlot || SourceSlot == this)
    {
        return false;
    }

    


    const int32 FromIndex = SourceSlot->SlotIndex;
    const int32 ToIndex = this->SlotIndex;

    
    if (!InventoryOwner || !InventoryOwner->InventoryRef) return false;
    if (!InventoryOwner->InventoryRef->InventoryItemsStruct.IsValidIndex(ToIndex)) return false;
    if (!SourceSlot->InventoryOwner || !SourceSlot->InventoryOwner->InventoryRef) return false;
    if (!SourceSlot->InventoryOwner->InventoryRef->InventoryItemsStruct.IsValidIndex(FromIndex)) return false;

    

    ////1. �������� �� �÷��̾� �κ��丮
    //if (SourceSlot->bIsChestInventory && !this->bIsChestInventory)
    //{
    //    if (InventoryOwner && InventoryOwner->InventoryRef && SourceSlot->InventoryOwner && SourceSlot->InventoryOwner->InventoryRef)
    //    {
    //        InventoryOwner->InventoryRef->InventoryItemsStruct[ToIndex] = SourceSlot->StoredData;
    //        SourceSlot->InventoryOwner->InventoryRef->RemoveItemAtStruct(FromIndex);

    //        UE_LOG(LogTemp, Log, TEXT("Moved item from Chest to Player Inventory"));
    //    }
    //}

    ////2. �÷��̾� �κ��丮 �� ��������
    //else if (!SourceSlot->bIsChestInventory && this->bIsChestInventory)
    //{
    //    if (InventoryOwner && InventoryOwner->InventoryRef && SourceSlot->InventoryOwner && SourceSlot->InventoryOwner->InventoryRef)
    //    {
    //        InventoryOwner->InventoryRef->InventoryItemsStruct[ToIndex] = SourceSlot->StoredData;
    //        SourceSlot->InventoryOwner->InventoryRef->RemoveItemAtStruct(FromIndex);

    //        UE_LOG(LogTemp, Log, TEXT("Moved item from Player Inventory to Chest"));
    //    }
    //}

    // 1. �������� �� �÷��̾� �κ��丮
    if (SourceSlot->bIsChestInventory && !this->bIsChestInventory)
    {

        


        // ���� RPC ȣ��
        InventoryOwner->InventoryRef->ServerMoveItem(
            SourceSlot->InventoryOwner->InventoryRef,
            FromIndex,
            ToIndex
        );
        UE_LOG(LogTemp, Log, TEXT("Client: Requested move from Chest to Player"));

       

    }
    // 2. �÷��̾� �κ��丮 �� ��������
    else if (!SourceSlot->bIsChestInventory && this->bIsChestInventory)
    {

        

        // ���� RPC ȣ��
        InventoryOwner->InventoryRef->ServerMoveItem(
            SourceSlot->InventoryOwner->InventoryRef,
            FromIndex,
            ToIndex
        );
        UE_LOG(LogTemp, Log, TEXT("Client: Requested move from Player to Chest"));

        
    }


    //  �κ��丮 �� ��Ű ����
    else if (!SourceSlot->bIsHotkeySlot && this->bIsHotkeySlot)
    {
        if (EquipmentOwner)
        {
            EquipmentOwner->SetSlot(ToIndex, SourceSlot->StoredData);

            if (SourceSlot->InventoryOwner && SourceSlot->InventoryOwner->InventoryRef)
            {
                SourceSlot->InventoryOwner->InventoryRef->RemoveItemAtStruct(FromIndex);
            }

            

            UE_LOG(LogTemp, Log, TEXT("Moved item from Inventory to Hotkey Slot %d"), ToIndex);
        }
    }


    // ���â �� �κ��丮
    else if (SourceSlot->bIsEquipmentSlot && !this->bIsEquipmentSlot)
    {
        if (InventoryOwner && InventoryOwner->InventoryRef && SourceSlot->EquipmentOwner)
        {
            InventoryOwner->InventoryRef->InventoryItemsStruct[ToIndex] = SourceSlot->StoredData;
            SourceSlot->EquipmentOwner->ClearSlot(FromIndex);
          
        }
        
    }
    // �κ��丮 �� ���â
    else if (!SourceSlot->bIsEquipmentSlot && this->bIsEquipmentSlot)
    {
        if (EquipmentOwner)
        {
            EquipmentOwner->SetSlot(ToIndex, SourceSlot->StoredData);

            if (SourceSlot->InventoryOwner && SourceSlot->InventoryOwner->InventoryRef)
            {
                SourceSlot->InventoryOwner->InventoryRef->RemoveItemAtStruct(FromIndex);
            }
        }
    }

    // ����â �� �κ��丮 (����)
    else if (SourceSlot->bIsShopSlot && !this->bIsShopSlot)
    {
        if (InventoryOwner && InventoryOwner->InventoryRef)
        {
            const int32 ItemPrice = SourceSlot->StoredData.Price;

            // ��Ƽ�÷��̾� ȯ��: ���� RPC ȣ��
            InventoryOwner->InventoryRef->ServerPurchaseItem(
                SourceSlot->StoredData,
                ToIndex,
                ItemPrice
            );

            UE_LOG(LogTemp, Log, TEXT("Client: Requested purchase of %s for %d gold"),
                *SourceSlot->StoredData.ItemName, ItemPrice);

            // �κ񿡼��� ���� ó�� (�̱��÷��̾� �Ǵ� �κ� ����)
            AMyDCharacter* Player = Cast<AMyDCharacter>(GetOwningPlayerPawn());
            if (!Player)
            {
                // �κ� ��Ȳ - GameInstance���� ���� ó��
                UDynamicDungeonInstance* GameInstance = Cast<UDynamicDungeonInstance>(GetGameInstance());
                if (GameInstance)
                {
                    if (GameInstance->LobbyGold >= ItemPrice)
                    {
                        // �κ񿡼��� ���÷� ó�� (������ �����Ƿ�)
                        GameInstance->LobbyGold -= ItemPrice;
                        InventoryOwner->InventoryRef->InventoryItemsStruct[ToIndex] = SourceSlot->StoredData;

                        // ��� UI ������Ʈ
                        if (ULobbyWidget* Lobby = GetTypedOuter<ULobbyWidget>())
                        {
                            if (Lobby->GoldWidgetInstance)
                            {
                                Lobby->GoldWidgetInstance->UpdateGoldAmount(GameInstance->LobbyGold);
                            }
                        }

                        UE_LOG(LogTemp, Log, TEXT("Lobby purchase successful: %s for %d gold"),
                            *SourceSlot->StoredData.ItemName, ItemPrice);
                    }
                    else
                    {
                        UE_LOG(LogTemp, Warning, TEXT("Not enough gold in lobby"));
                    }
                }
            }
        }
    }

    // �κ��丮 �� ����â (�Ǹ�)
    else if (!SourceSlot->bIsShopSlot && this->bIsShopSlot)
    {
        if (SourceSlot->InventoryOwner && SourceSlot->InventoryOwner->InventoryRef)
        {
            const int32 ItemPrice = SourceSlot->StoredData.Price;
            const int32 SellPrice = FMath::FloorToInt(ItemPrice * 0.6f);

            // ��Ƽ�÷��̾� ȯ��: ���� RPC ȣ��
            SourceSlot->InventoryOwner->InventoryRef->ServerSellItem(
                SourceSlot->SlotIndex,
                SellPrice
            );

            UE_LOG(LogTemp, Log, TEXT("Client: Requested sale of %s for %d gold"),
                *SourceSlot->StoredData.ItemName, SellPrice);

            // �κ񿡼��� ���� ó��
            AMyDCharacter* Player = Cast<AMyDCharacter>(GetOwningPlayerPawn());
            if (!Player)
            {
                UDynamicDungeonInstance* GameInstance = Cast<UDynamicDungeonInstance>(GetGameInstance());
                if (GameInstance)
                {
                    // �κ񿡼��� ���÷� ó��
                    GameInstance->LobbyGold += SellPrice;
                    SourceSlot->SetItemData(FItemData());

                    // ��� UI ������Ʈ
                    if (GameInstance->LobbyWidgetInstance &&
                        GameInstance->LobbyWidgetInstance->GoldWidgetInstance)
                    {
                        GameInstance->LobbyWidgetInstance->GoldWidgetInstance->UpdateGoldAmount(
                            GameInstance->LobbyGold
                        );
                    }

                    UE_LOG(LogTemp, Log, TEXT("Lobby sale successful: received %d gold"), SellPrice);
                }
            }

            return true;
        }
    }


    // ���� �׷� �� ����
    else
    {
        if (bIsEquipmentSlot)
        {
            if (EquipmentOwner)
            {
                EquipmentOwner->SwapSlots(FromIndex, ToIndex);
            }
        }
        else
        {
            if (InventoryOwner && InventoryOwner->InventoryRef)
            {
                InventoryOwner->InventoryRef->InventoryItemsStruct.Swap(FromIndex, ToIndex);
            }
        }
    }

    if (InventoryOwner) InventoryOwner->RefreshInventoryStruct();
    if (EquipmentOwner) EquipmentOwner->RefreshEquipmentSlots();

    if (SourceSlot->InventoryOwner)
    {
        SourceSlot->InventoryOwner->RefreshInventoryStruct();
    }

    return true;
}
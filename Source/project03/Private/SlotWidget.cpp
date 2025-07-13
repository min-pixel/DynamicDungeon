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

    // ����â �� â�� (����) ���� �ȿ� �߰�
    else if (SourceSlot->bIsShopSlot && this->bIsChestInventory && !this->bIsShopSlot)
    {
        UE_LOG(LogTemp, Error, TEXT("Entering Shop��Storage logic"));

        if (InventoryOwner && InventoryOwner->InventoryRef)
        {
            UE_LOG(LogTemp, Error, TEXT("InventoryOwner and InventoryRef are valid"));

            const int32 ItemPrice = SourceSlot->StoredData.Price;
            UE_LOG(LogTemp, Error, TEXT("Item price: %d"), ItemPrice);

            // �κ񿡼� ���� ó��
            UDynamicDungeonInstance* GameInstance = Cast<UDynamicDungeonInstance>(GetGameInstance());
            if (GameInstance)
            {
                UE_LOG(LogTemp, Error, TEXT("GameInstance found"));
                UE_LOG(LogTemp, Error, TEXT("Current LobbyGold: %d"), GameInstance->LobbyGold);
                UE_LOG(LogTemp, Error, TEXT("Required price: %d"), ItemPrice);

                if (GameInstance->LobbyGold >= ItemPrice)
                {
                    UE_LOG(LogTemp, Error, TEXT("Sufficient gold - processing purchase"));

                    // ��� ���� �� ������ �߰�
                    GameInstance->LobbyGold -= ItemPrice;
                    UE_LOG(LogTemp, Error, TEXT("Gold after purchase: %d"), GameInstance->LobbyGold);

                    // ���� ��ȿ�� ��Ȯ��
                    if (InventoryOwner->InventoryRef->InventoryItemsStruct.IsValidIndex(ToIndex))
                    {
                        UE_LOG(LogTemp, Error, TEXT("ToIndex %d is valid"), ToIndex);

                        InventoryOwner->InventoryRef->InventoryItemsStruct[ToIndex] = SourceSlot->StoredData;
                        UE_LOG(LogTemp, Error, TEXT("Item added to storage"));

                        // UI ���ΰ�ħ
                        InventoryOwner->RefreshInventoryStruct();
                        UE_LOG(LogTemp, Error, TEXT("Storage UI refreshed"));

                        // ��� UI ������Ʈ
                        if (ULobbyWidget* Lobby = GetTypedOuter<ULobbyWidget>())
                        {
                            UE_LOG(LogTemp, Error, TEXT("LobbyWidget found"));
                            if (Lobby->GoldWidgetInstance)
                            {
                                UE_LOG(LogTemp, Error, TEXT("GoldWidget found - updating"));
                                Lobby->GoldWidgetInstance->UpdateGoldAmount(GameInstance->LobbyGold);
                                UE_LOG(LogTemp, Error, TEXT("Gold UI updated"));
                            }
                            else
                            {
                                UE_LOG(LogTemp, Error, TEXT("GoldWidget is NULL"));
                            }
                        }
                        else
                        {
                            UE_LOG(LogTemp, Error, TEXT("LobbyWidget not found"));
                        }

                        UE_LOG(LogTemp, Error, TEXT("Shop �� Storage purchase COMPLETED!"));
                    }
                    else
                    {
                        UE_LOG(LogTemp, Error, TEXT("ToIndex %d is INVALID"), ToIndex);
                    }
                }
                else
                {
                    UE_LOG(LogTemp, Error, TEXT("Not enough gold: have %d, need %d"),
                        GameInstance->LobbyGold, ItemPrice);
                }
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("GameInstance not found"));
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("InventoryOwner or InventoryRef is NULL"));
        }
    }

    //�Ǹ� - ���� �ڵ�� ������ ������ ���
    else if (!SourceSlot->bIsShopSlot && this->bIsShopSlot)
    {
        if (SourceSlot->InventoryOwner && SourceSlot->InventoryOwner->InventoryRef)
        {
            const int32 ItemPrice = SourceSlot->StoredData.Price;
            const int32 SellPrice = FMath::FloorToInt(ItemPrice * 0.6f);

            UDynamicDungeonInstance* GameInstance = Cast<UDynamicDungeonInstance>(GetGameInstance());
            if (GameInstance)
            {
                // ���ſ� ����: ��� ����
                GameInstance->LobbyGold += SellPrice;

                // ���ſ� ����: ������ ����
                SourceSlot->InventoryOwner->InventoryRef->InventoryItemsStruct[SourceSlot->SlotIndex] = FItemData();

                // ���ſ� ����: UI ���ΰ�ħ (SourceSlot�� �ƴ� InventoryOwner ����)
                if (SourceSlot->InventoryOwner)
                {
                    SourceSlot->InventoryOwner->RefreshInventoryStruct();
                }

                // ���ſ� ������ ����: SourceSlot�� �ƴ� �Ϲ� ���Կ��� LobbyWidget ã��
                if (ULobbyWidget* Lobby = SourceSlot->InventoryOwner->GetTypedOuter<ULobbyWidget>())
                {
                    if (Lobby->GoldWidgetInstance)
                    {
                        Lobby->GoldWidgetInstance->UpdateGoldAmount(GameInstance->LobbyGold);
                        UE_LOG(LogTemp, Warning, TEXT("Gold UI updated via SourceSlot->InventoryOwner"));
                    }
                }
                // ���: ���� ��ĵ� �õ�
                /*else if (ULobbyWidget* Lobby = GetTypedOuter<ULobbyWidget>())
                {
                    if (Lobby->GoldWidgetInstance)
                    {
                        Lobby->GoldWidgetInstance->UpdateGoldAmount(GameInstance->LobbyGold);
                        UE_LOG(LogTemp, Warning, TEXT("Gold UI updated via GetTypedOuter"));
                    }
                }*/

                UE_LOG(LogTemp, Log, TEXT("Sale successful: +%d gold, total: %d"), SellPrice, GameInstance->LobbyGold);
                return true;
            }
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
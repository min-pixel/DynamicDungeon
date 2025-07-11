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
//        ItemIcon->SetColorAndOpacity(FLinearColor::White); // 아이콘 보이게
//    }
//    else
//    {
//        // 빈 슬롯용 기본 회색 칠하기
//        FSlateBrush EmptyBrush;
//        EmptyBrush.TintColor = FLinearColor(0.2f, 0.2f, 0.2f, 0.8f); // 회색 반투명
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
//    DragOp->DefaultDragVisual = this;  // 임시: 현재 슬롯 전체가 따라다님
//    DragOp->Pivot = EDragPivot::MouseDown;
//
//    OutOperation = DragOp;
//
//    SetVisibility(ESlateVisibility::Hidden);  // 시각적으로 숨김 (드래그 중 표시 방지)
//}


//void USlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
//{
//    if (!StoredItem) return;
//
//    UDragDropOperation* DragOp = NewObject<UDragDropOperation>();
//    DragOp->Payload = this; // 나중에 사용할 수 있도록 현재 슬롯 참조 전달
//    //DragOp->DefaultDragVisual = this; // 현재 위젯 그대로 보이게
//
//     // 새로운 시각용 위젯을 생성
//    USlotWidget* DragVisual = CreateWidget<USlotWidget>(GetWorld(), GetClass());
//    DragVisual->SetItem(StoredItem);  // 시각적으로 동일하게
//    DragVisual->SetVisibility(ESlateVisibility::SelfHitTestInvisible); // 클릭 막지 않게
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
//    // 인덱스 얻기 (나중에 명확한 슬롯 인덱스 시스템이 있다면 그걸로 교체)
//    int32 FromIndex = SourceSlot->SlotIndex;
//    int32 ToIndex = this->SlotIndex;
//
//    if (!InventoryOwner->InventoryRef) return false;
//
//
//    // 스왑
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


    // 툴팁 위젯 연결
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

    

    ////1. 보물상자 → 플레이어 인벤토리
    //if (SourceSlot->bIsChestInventory && !this->bIsChestInventory)
    //{
    //    if (InventoryOwner && InventoryOwner->InventoryRef && SourceSlot->InventoryOwner && SourceSlot->InventoryOwner->InventoryRef)
    //    {
    //        InventoryOwner->InventoryRef->InventoryItemsStruct[ToIndex] = SourceSlot->StoredData;
    //        SourceSlot->InventoryOwner->InventoryRef->RemoveItemAtStruct(FromIndex);

    //        UE_LOG(LogTemp, Log, TEXT("Moved item from Chest to Player Inventory"));
    //    }
    //}

    ////2. 플레이어 인벤토리 → 보물상자
    //else if (!SourceSlot->bIsChestInventory && this->bIsChestInventory)
    //{
    //    if (InventoryOwner && InventoryOwner->InventoryRef && SourceSlot->InventoryOwner && SourceSlot->InventoryOwner->InventoryRef)
    //    {
    //        InventoryOwner->InventoryRef->InventoryItemsStruct[ToIndex] = SourceSlot->StoredData;
    //        SourceSlot->InventoryOwner->InventoryRef->RemoveItemAtStruct(FromIndex);

    //        UE_LOG(LogTemp, Log, TEXT("Moved item from Player Inventory to Chest"));
    //    }
    //}

    // 1. 보물상자 → 플레이어 인벤토리
    if (SourceSlot->bIsChestInventory && !this->bIsChestInventory)
    {

        


        // 서버 RPC 호출
        InventoryOwner->InventoryRef->ServerMoveItem(
            SourceSlot->InventoryOwner->InventoryRef,
            FromIndex,
            ToIndex
        );
        UE_LOG(LogTemp, Log, TEXT("Client: Requested move from Chest to Player"));

       

    }
    // 2. 플레이어 인벤토리 → 보물상자
    else if (!SourceSlot->bIsChestInventory && this->bIsChestInventory)
    {

        

        // 서버 RPC 호출
        InventoryOwner->InventoryRef->ServerMoveItem(
            SourceSlot->InventoryOwner->InventoryRef,
            FromIndex,
            ToIndex
        );
        UE_LOG(LogTemp, Log, TEXT("Client: Requested move from Player to Chest"));

        
    }


    //  인벤토리 → 핫키 슬롯
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


    // 장비창 → 인벤토리
    else if (SourceSlot->bIsEquipmentSlot && !this->bIsEquipmentSlot)
    {
        if (InventoryOwner && InventoryOwner->InventoryRef && SourceSlot->EquipmentOwner)
        {
            InventoryOwner->InventoryRef->InventoryItemsStruct[ToIndex] = SourceSlot->StoredData;
            SourceSlot->EquipmentOwner->ClearSlot(FromIndex);
          
        }
        
    }
    // 인벤토리 → 장비창
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

    // 상점창 → 인벤토리 (구매)
    else if (SourceSlot->bIsShopSlot && !this->bIsShopSlot)
    {
        if (InventoryOwner && InventoryOwner->InventoryRef)
        {
            const int32 ItemPrice = SourceSlot->StoredData.Price;

            // 멀티플레이어 환경: 서버 RPC 호출
            InventoryOwner->InventoryRef->ServerPurchaseItem(
                SourceSlot->StoredData,
                ToIndex,
                ItemPrice
            );

            UE_LOG(LogTemp, Log, TEXT("Client: Requested purchase of %s for %d gold"),
                *SourceSlot->StoredData.ItemName, ItemPrice);

            // 로비에서의 로컬 처리 (싱글플레이어 또는 로비 전용)
            AMyDCharacter* Player = Cast<AMyDCharacter>(GetOwningPlayerPawn());
            if (!Player)
            {
                // 로비 상황 - GameInstance에서 직접 처리
                UDynamicDungeonInstance* GameInstance = Cast<UDynamicDungeonInstance>(GetGameInstance());
                if (GameInstance)
                {
                    if (GameInstance->LobbyGold >= ItemPrice)
                    {
                        // 로비에서는 로컬로 처리 (서버가 없으므로)
                        GameInstance->LobbyGold -= ItemPrice;
                        InventoryOwner->InventoryRef->InventoryItemsStruct[ToIndex] = SourceSlot->StoredData;

                        // 골드 UI 업데이트
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

    // 인벤토리 → 상점창 (판매)
    else if (!SourceSlot->bIsShopSlot && this->bIsShopSlot)
    {
        if (SourceSlot->InventoryOwner && SourceSlot->InventoryOwner->InventoryRef)
        {
            const int32 ItemPrice = SourceSlot->StoredData.Price;
            const int32 SellPrice = FMath::FloorToInt(ItemPrice * 0.6f);

            // 멀티플레이어 환경: 서버 RPC 호출
            SourceSlot->InventoryOwner->InventoryRef->ServerSellItem(
                SourceSlot->SlotIndex,
                SellPrice
            );

            UE_LOG(LogTemp, Log, TEXT("Client: Requested sale of %s for %d gold"),
                *SourceSlot->StoredData.ItemName, SellPrice);

            // 로비에서의 로컬 처리
            AMyDCharacter* Player = Cast<AMyDCharacter>(GetOwningPlayerPawn());
            if (!Player)
            {
                UDynamicDungeonInstance* GameInstance = Cast<UDynamicDungeonInstance>(GetGameInstance());
                if (GameInstance)
                {
                    // 로비에서는 로컬로 처리
                    GameInstance->LobbyGold += SellPrice;
                    SourceSlot->SetItemData(FItemData());

                    // 골드 UI 업데이트
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


    // 같은 그룹 간 스왑
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
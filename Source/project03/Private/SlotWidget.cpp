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

    // 상점창 → 창고 (구매) 조건 안에 추가
    else if (SourceSlot->bIsShopSlot && this->bIsChestInventory && !this->bIsShopSlot)
    {
        UE_LOG(LogTemp, Error, TEXT("Entering Shop→Storage logic"));

        if (InventoryOwner && InventoryOwner->InventoryRef)
        {
            UE_LOG(LogTemp, Error, TEXT("InventoryOwner and InventoryRef are valid"));

            const int32 ItemPrice = SourceSlot->StoredData.Price;
            UE_LOG(LogTemp, Error, TEXT("Item price: %d"), ItemPrice);

            // 로비에서 로컬 처리
            UDynamicDungeonInstance* GameInstance = Cast<UDynamicDungeonInstance>(GetGameInstance());
            if (GameInstance)
            {
                UE_LOG(LogTemp, Error, TEXT("GameInstance found"));
                UE_LOG(LogTemp, Error, TEXT("Current LobbyGold: %d"), GameInstance->LobbyGold);
                UE_LOG(LogTemp, Error, TEXT("Required price: %d"), ItemPrice);

                if (GameInstance->LobbyGold >= ItemPrice)
                {
                    UE_LOG(LogTemp, Error, TEXT("Sufficient gold - processing purchase"));

                    // 골드 차감 및 아이템 추가
                    GameInstance->LobbyGold -= ItemPrice;
                    UE_LOG(LogTemp, Error, TEXT("Gold after purchase: %d"), GameInstance->LobbyGold);

                    // 슬롯 유효성 재확인
                    if (InventoryOwner->InventoryRef->InventoryItemsStruct.IsValidIndex(ToIndex))
                    {
                        UE_LOG(LogTemp, Error, TEXT("ToIndex %d is valid"), ToIndex);

                        InventoryOwner->InventoryRef->InventoryItemsStruct[ToIndex] = SourceSlot->StoredData;
                        UE_LOG(LogTemp, Error, TEXT("Item added to storage"));

                        // UI 새로고침
                        InventoryOwner->RefreshInventoryStruct();
                        UE_LOG(LogTemp, Error, TEXT("Storage UI refreshed"));

                        // 골드 UI 업데이트
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

                        UE_LOG(LogTemp, Error, TEXT("Shop → Storage purchase COMPLETED!"));
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

    //판매 - 구매 코드와 완전히 동일한 방식
    else if (!SourceSlot->bIsShopSlot && this->bIsShopSlot)
    {
        if (SourceSlot->InventoryOwner && SourceSlot->InventoryOwner->InventoryRef)
        {
            const int32 ItemPrice = SourceSlot->StoredData.Price;
            const int32 SellPrice = FMath::FloorToInt(ItemPrice * 0.6f);

            UDynamicDungeonInstance* GameInstance = Cast<UDynamicDungeonInstance>(GetGameInstance());
            if (GameInstance)
            {
                // 구매와 동일: 골드 증가
                GameInstance->LobbyGold += SellPrice;

                // 구매와 동일: 아이템 제거
                SourceSlot->InventoryOwner->InventoryRef->InventoryItemsStruct[SourceSlot->SlotIndex] = FItemData();

                // 구매와 동일: UI 새로고침 (SourceSlot이 아닌 InventoryOwner 기준)
                if (SourceSlot->InventoryOwner)
                {
                    SourceSlot->InventoryOwner->RefreshInventoryStruct();
                }

                // 구매와 완전히 동일: SourceSlot이 아닌 일반 슬롯에서 LobbyWidget 찾기
                if (ULobbyWidget* Lobby = SourceSlot->InventoryOwner->GetTypedOuter<ULobbyWidget>())
                {
                    if (Lobby->GoldWidgetInstance)
                    {
                        Lobby->GoldWidgetInstance->UpdateGoldAmount(GameInstance->LobbyGold);
                        UE_LOG(LogTemp, Warning, TEXT("Gold UI updated via SourceSlot->InventoryOwner"));
                    }
                }
                // 백업: 기존 방식도 시도
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
// Fill out your copyright notice in the Description page of Project Settings.


#include "SlotWidget.h"
#include "Components/Image.h"
#include "InventoryWidget.h"
#include "InventoryComponent.h"
#include "Blueprint/DragDropOperation.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Item.h"

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
//FReply USlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
//{
//    if (InMouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
//    {
//        return UWidgetBlueprintLibrary::DetectDragIfPressed(InMouseEvent, this, EKeys::LeftMouseButton).NativeReply;
//    }
//    return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
//}

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
}

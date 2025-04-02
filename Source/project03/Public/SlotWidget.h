// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Item.h"
#include "SlotWidget.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT03_API USlotWidget : public UUserWidget
{
	GENERATED_BODY()
public:

    UPROPERTY(meta = (BindWidget))
    class UImage* ItemIcon;

   /* UFUNCTION(BlueprintCallable)
    void SetItem(class AItem* InItem);*/


    UPROPERTY()
    class UInventoryWidget* InventoryOwner;

    UPROPERTY()
    int32 SlotIndex;

    UFUNCTION(BlueprintCallable)
    AItem* GetStoredItem() const { return StoredItem; }

    UPROPERTY()
    FItemData StoredData;

    void SetItemData(const FItemData& NewData);
   

   /* virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;

    virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;*/


protected:
    AItem* StoredItem;
   

};

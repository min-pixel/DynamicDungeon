// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Item.h"
#include "ItemTooltipWidget.h"
#include "UCharacterHUDWidget.h"
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

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn))
    int32 SlotIndex;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn))
    bool bIsEquipmentSlot = false; // true면 장비창, false면 인벤토리


    UPROPERTY()
    class UInventoryWidget* InventoryOwner;

    UPROPERTY(BlueprintReadWrite)
    class UEquipmentWidget* EquipmentOwner;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bIsHotkeySlot = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tooltip")
    TSubclassOf<class UItemTooltipWidget> TooltipWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 HotkeyIndex = -1; // 0 ~ 4 (1번 슬롯 ~ 5번 슬롯)

    UFUNCTION(BlueprintCallable)
    AItem* GetStoredItem() const { return StoredItem; }

    UPROPERTY()
    FItemData StoredData;

    void SetItemData(const FItemData& NewData);
   
    virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;

    virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;


    virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
    /*virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;

    virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;*/

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn))
    bool bIsChestInventory = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ExposeOnSpawn))
    bool bIsShopSlot = false;

   


protected:
    AItem* StoredItem;
   

};

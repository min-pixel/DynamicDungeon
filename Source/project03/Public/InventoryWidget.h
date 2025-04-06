// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SlotWidget.h"
#include "InventoryWidget.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT03_API UInventoryWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
    TSubclassOf<class USlotWidget> SlotWidgetClass;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
    class UInventoryComponent* InventoryRef;

    UPROPERTY(meta = (BindWidget))
    class UWrapBox* ItemContainer; // VerticalBox, WrapBox µî

    /*UFUNCTION(BlueprintCallable)
    void RefreshInventory();*/

    void RefreshInventoryStruct();

    //virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Inventory")
    bool bIsChestInventory = false;


};

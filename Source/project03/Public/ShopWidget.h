// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ItemDataD.h"
#include "SlotWidget.h"
#include "Components/WrapBox.h"
#include "ShopWidget.generated.h"

/**
 * 
 */



UCLASS()
class PROJECT03_API UShopWidget : public UUserWidget
{
	GENERATED_BODY()
public:
    // ���Ե��� ǥ���� WrapBox
    UPROPERTY(meta = (BindWidget))
    class UWrapBox* ShopItemContainer;

    // ���� ������ ���
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
    TArray<FItemData> ShopItemList;

    // ���� ������ Ŭ����
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
    TSubclassOf<USlotWidget> SlotWidgetClass;

    // ������ �������� ���� �κ��丮
    UPROPERTY(BlueprintReadWrite)
    class UInventoryComponent* TargetInventory;

    UPROPERTY(BlueprintReadWrite)
    class UInventoryComponent* StorageInventory;

    UPROPERTY()
    class UInventoryWidget* ShopDummyWidget;

    void PopulateShopItems();

};

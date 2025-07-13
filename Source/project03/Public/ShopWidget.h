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
    // 슬롯들을 표시할 WrapBox
    UPROPERTY(meta = (BindWidget))
    class UWrapBox* ShopItemContainer;

    // 상점 아이템 목록
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
    TArray<FItemData> ShopItemList;

    // 슬롯 생성용 클래스
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Shop")
    TSubclassOf<USlotWidget> SlotWidgetClass;

    // 구매한 아이템을 넣을 인벤토리
    UPROPERTY(BlueprintReadWrite)
    class UInventoryComponent* TargetInventory;

    UPROPERTY(BlueprintReadWrite)
    class UInventoryComponent* StorageInventory;

    UPROPERTY()
    class UInventoryWidget* ShopDummyWidget;

    void PopulateShopItems();

};

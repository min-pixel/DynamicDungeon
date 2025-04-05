// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SlotWidget.h"
#include "Components/WrapBox.h"
#include "EquipmentWidget.generated.h"

#define EQUIP_SLOT_WEAPON 0
#define EQUIP_SLOT_HELMET 1
#define EQUIP_SLOT_CHEST  2
#define EQUIP_SLOT_LEG    3

/**
 * 
 */
UCLASS()
class PROJECT03_API UEquipmentWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:

    UPROPERTY(meta = (BindWidget))
    class UWrapBox* EquipmentSlotContainer;

    //// 무기 슬롯 (양손 무기 포함)
    //UPROPERTY(meta = (BindWidget))
    //USlotWidget* WeaponSlot;

    //// 갑옷 슬롯들
    //UPROPERTY(meta = (BindWidget))
    //USlotWidget* HelmetSlot;

    //UPROPERTY(meta = (BindWidget))
    //USlotWidget* ChestSlot;

    //UPROPERTY(meta = (BindWidget))
    //USlotWidget* LegSlot;

    // 초기화용 함수 (선택사항)
    void RefreshEquipmentSlots();

    void SetSlot(int32 Index, const FItemData& ItemData);
    void ClearSlot(int32 Index);
    void SwapSlots(int32 From, int32 To);

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FItemData> EquipmentSlots;

    UPROPERTY(BlueprintReadWrite)
    class UInventoryWidget* InventoryOwner;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "UI")
    TSubclassOf<class USlotWidget> SlotWidgetClass;

};

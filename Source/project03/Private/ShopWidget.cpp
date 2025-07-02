// Fill out your copyright notice in the Description page of Project Settings.


#include "ShopWidget.h"
#include "InventoryComponent.h"
#include "SlotWidget.h"
#include "Components/WrapBox.h"
#include "Blueprint/UserWidget.h"
#include "LobbyWidget.h"

void UShopWidget::PopulateShopItems()
{
    if (!ShopItemContainer || !SlotWidgetClass) return;

    ShopItemContainer->ClearChildren();

    // 1) 원본 리스트에서 등급 아이템은 3단계(C/B/A)로, 그 외는 그대로 확장
    TArray<FItemData> ExpandedList;
    for (const FItemData& Data : ShopItemList)
    {
        // Armor 또는 Weapon 타입이면 등급 0(C)~2(A)까지 복제
        if (Data.ItemType == EItemType::Armor || Data.ItemType == EItemType::Weapon)
        {
            for (uint8 G = 0; G <= 2; ++G)
            {
                FItemData NewData = Data;
                NewData.Grade = G;
                
                // 기본 가격
                NewData.Price = Data.Price;

                // 등급 계수 곱하기
                float Multiplier = 1.0f;
                switch (G)
                {
                case 1: // B
                    Multiplier = 1.5f;
                    break;
                case 2: // A
                    Multiplier = 2.0f;
                    break;
                default: // C
                    Multiplier = 1.0f;
                    break;
                }

                NewData.Price = FMath::RoundToInt(NewData.Price * Multiplier);

                static const TCHAR* GradeNames[] = { TEXT("C"), TEXT("B"), TEXT("A") };
                NewData.ItemName = FString::Printf(TEXT("%s [%sGrade]"), *Data.ItemName, GradeNames[G]);

                ExpandedList.Add(NewData);
            }
        }
        else
        {
            ExpandedList.Add(Data);
        }
    }

    // 2) 슬롯 개수 계산
    const int32 NumItems = ExpandedList.Num();
    const int32 NumExtra = 10;  // 여분 슬롯 개수
    const int32 TotalSlots = NumItems + NumExtra;

    // 3) 슬롯 생성 및 데이터 세팅
    for (int32 i = 0; i < TotalSlots; ++i)
    {
        USlotWidget* NewSlot = CreateWidget<USlotWidget>(this, SlotWidgetClass);
        if (!NewSlot) continue;

        if (i < NumItems)
        {
            NewSlot->SetItemData(ExpandedList[i]);
        }
        else
        {
            NewSlot->SetItemData(FItemData());  // 빈 슬롯
        }

        NewSlot->bIsShopSlot = true;
        NewSlot->bIsChestInventory = true;
        NewSlot->InventoryOwner = nullptr;
        NewSlot->SlotIndex = i;
        ShopItemContainer->AddChild(NewSlot);
    }
}


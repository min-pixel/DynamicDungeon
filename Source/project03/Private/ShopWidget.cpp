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

    // 1) ���� ����Ʈ���� ��� �������� 3�ܰ�(C/B/A)��, �� �ܴ� �״�� Ȯ��
    TArray<FItemData> ExpandedList;
    for (const FItemData& Data : ShopItemList)
    {
        // Armor �Ǵ� Weapon Ÿ���̸� ��� 0(C)~2(A)���� ����
        if (Data.ItemType == EItemType::Armor || Data.ItemType == EItemType::Weapon)
        {
            for (uint8 G = 0; G <= 2; ++G)
            {
                FItemData NewData = Data;
                NewData.Grade = G;
                
                // �⺻ ����
                NewData.Price = Data.Price;

                // ��� ��� ���ϱ�
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

    // 2) ���� ���� ���
    const int32 NumItems = ExpandedList.Num();
    const int32 NumExtra = 10;  // ���� ���� ����
    const int32 TotalSlots = NumItems + NumExtra;

    // 3) ���� ���� �� ������ ����
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
            NewSlot->SetItemData(FItemData());  // �� ����
        }

        NewSlot->bIsShopSlot = true;
        NewSlot->bIsChestInventory = true;
        NewSlot->InventoryOwner = nullptr;
        NewSlot->SlotIndex = i;
        ShopItemContainer->AddChild(NewSlot);
    }
}


#include "ItemTooltipWidget.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

FString GetGradeLetter(uint8 Grade)
{
    switch (Grade)
    {
    case 2: return TEXT("A");
    case 1: return TEXT("B");
    case 0: return TEXT("C");
    default: return TEXT("?");
    }
}

void UItemTooltipWidget::InitWithItemData(const FItemData& Data)
{
    // 아이콘
    if (ItemIconImage && Data.ItemIcon)
    {
        ItemIconImage->SetBrushFromTexture(Data.ItemIcon);
    }

    // 타입
    if (ItemTypeText)
    {
        ItemTypeText->SetText(UEnum::GetDisplayValueAsText(Data.ItemType));
    }

    // 이름 (Armor는 "[X등급]" 접미 제거)
    if (ItemNameText)
    {
        FString Name = Data.ItemName;
        if (Data.ItemType == EItemType::Armor|| Data.ItemType == EItemType::Weapon)
        {
            int32 BracketPos;
            if (Name.FindLastChar('[', BracketPos))
            {
                // "[X등급]" 직전 공백까지 잘라냄
                Name = Name.Left(BracketPos - 1);
            }
        }
        ItemNameText->SetText(FText::FromString(Name));
    }

    // 등급: Weapon 또는 Armor만 보여주고, 그 외는 숨김
    if (GradeText)
    {
        if (Data.ItemType == EItemType::Weapon || Data.ItemType == EItemType::Armor)
        {
            GradeText->SetVisibility(ESlateVisibility::Visible);
            const FString GradeStr = GetGradeLetter(Data.Grade);
            GradeText->SetText(
                FText::Format(
                    NSLOCTEXT("Tooltip", "Grade", "Grade: {0}"),
                    FText::FromString(GradeStr)
                )
            );
        }
        else
        {
            GradeText->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    // 가격
    if (PriceText)
    {
        PriceText->SetText(
            FText::Format(
                NSLOCTEXT("Tooltip", "Price", "Price: {0} G"),
                Data.Price
            )
        );
    }
}

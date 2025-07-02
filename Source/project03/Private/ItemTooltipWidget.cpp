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
    // ������
    if (ItemIconImage && Data.ItemIcon)
    {
        ItemIconImage->SetBrushFromTexture(Data.ItemIcon);
    }

    // Ÿ��
    if (ItemTypeText)
    {
        ItemTypeText->SetText(UEnum::GetDisplayValueAsText(Data.ItemType));
    }

    // �̸� (Armor�� "[X���]" ���� ����)
    if (ItemNameText)
    {
        FString Name = Data.ItemName;
        if (Data.ItemType == EItemType::Armor|| Data.ItemType == EItemType::Weapon)
        {
            int32 BracketPos;
            if (Name.FindLastChar('[', BracketPos))
            {
                // "[X���]" ���� ������� �߶�
                Name = Name.Left(BracketPos - 1);
            }
        }
        ItemNameText->SetText(FText::FromString(Name));
    }

    // ���: Weapon �Ǵ� Armor�� �����ְ�, �� �ܴ� ����
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

    // ����
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

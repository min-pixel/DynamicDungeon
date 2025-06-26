// Fill out your copyright notice in the Description page of Project Settings.

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "ItemTooltipWidget.h"

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
    if (ItemIconImage && Data.ItemIcon)
        ItemIconImage->SetBrushFromTexture(Data.ItemIcon);

    if (ItemTypeText)
        ItemTypeText->SetText(UEnum::GetDisplayValueAsText(Data.ItemType));

    if (ItemNameText)
        ItemNameText->SetText(FText::FromString(Data.ItemName));

    if (GradeText)
    {
        FString GradeStr = GetGradeLetter(Data.Grade);
        GradeText->SetText(FText::Format(NSLOCTEXT("Tooltip", "Grade", "Grade: {0}"), FText::FromString(GradeStr)));
    }

    if (PriceText)
        PriceText->SetText(FText::Format(NSLOCTEXT("Tooltip", "Price", "Price: {0} G"), Data.Price));
}


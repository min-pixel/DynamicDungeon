// Fill out your copyright notice in the Description page of Project Settings.


#include "GoldWidget.h"
#include "Components/TextBlock.h"
#include "DynamicDungeonInstance.h"
#include "Kismet/GameplayStatics.h"
#include "MyDCharacter.h"

void UGoldWidget::NativeConstruct()
{
    Super::NativeConstruct();
    UpdateGoldAmount(0);  // �ʱⰪ
}

void UGoldWidget::UpdateGoldAmount(int32 NewGold)
{


    if (GoldText)
    {
        GoldText->SetText(FText::FromString(FString::Printf(TEXT("GOLD: %dG"), NewGold)));
    }
    else 
    {
        UE_LOG(LogTemp, Error, TEXT("GoldText is null! Check UMG binding."));
        
    }
}


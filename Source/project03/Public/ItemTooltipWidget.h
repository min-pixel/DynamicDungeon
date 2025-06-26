// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ItemDataD.h"
#include "ItemTooltipWidget.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT03_API UItemTooltipWidget : public UUserWidget
{
	GENERATED_BODY()
public:
    // 아이템 데이터 바인딩용 함수 (BP에서도 오버라이드 가능)
    UFUNCTION(BlueprintCallable)
    void InitWithItemData(const FItemData& Data);

    UPROPERTY(meta = (BindWidget))
    class UImage* ItemIconImage;
    UPROPERTY(meta = (BindWidget))
    class UTextBlock* ItemTypeText;
    UPROPERTY(meta = (BindWidget))
    class UTextBlock* ItemNameText;
    UPROPERTY(meta = (BindWidget))
    class UTextBlock* GradeText;
    UPROPERTY(meta = (BindWidget))
    class UTextBlock* PriceText;
};

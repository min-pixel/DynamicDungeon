// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GoldWidget.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT03_API UGoldWidget : public UUserWidget
{
	GENERATED_BODY()
	
    public:
        UFUNCTION(BlueprintCallable)
        void UpdateGoldAmount(int32 NewGold);

  

    protected:
        virtual void NativeConstruct() override;

        UPROPERTY(meta = (BindWidget))
        class UTextBlock* GoldText;

};

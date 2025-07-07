// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "WaitForPlayersWidget.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT03_API UWaitForPlayersWidget : public UUserWidget
{
	GENERATED_BODY()

public:
    UPROPERTY(meta = (BindWidget))
    class UTextBlock* StatusText;

    UFUNCTION(BlueprintCallable)
    void UpdateStatus(int32 ReadyCount, int32 TotalCount);
};

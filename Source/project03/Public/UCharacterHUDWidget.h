// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UCharacterHUDWidget.generated.h"

/**
 * 
 */
UCLASS()
class PROJECT03_API UUCharacterHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
    /** 체력 UI 업데이트 */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void UpdateHealth(float CurrentHealth, float MaxHealth);

    /** 마나 UI 업데이트 */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void UpdateMana(float CurrentMana, float MaxMana);

    /** 체력바 (BP에서 바인딩) */
    UPROPERTY(meta = (BindWidget))
    class UProgressBar* HealthProgressBar;

    /** 마나바 (BP에서 바인딩) */
    UPROPERTY(meta = (BindWidget))
    class UProgressBar* ManaProgressBar;
};

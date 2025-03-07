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
    /** ü�� UI ������Ʈ */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void UpdateHealth(float CurrentHealth, float MaxHealth);

    /** ���� UI ������Ʈ */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void UpdateMana(float CurrentMana, float MaxMana);

    /** ü�¹� (BP���� ���ε�) */
    UPROPERTY(meta = (BindWidget))
    class UProgressBar* HealthProgressBar;

    /** ������ (BP���� ���ε�) */
    UPROPERTY(meta = (BindWidget))
    class UProgressBar* ManaProgressBar;
};

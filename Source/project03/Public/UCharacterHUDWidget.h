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

    UFUNCTION(BlueprintCallable, Category = "HUD")
    void UpdateStamina(float CurrentStamina, float MaxStamina);

    /** 체력바 (BP에서 바인딩) */
    UPROPERTY(meta = (BindWidget))
    class UProgressBar* HealthProgressBar;

    /** 마나바 (BP에서 바인딩) */
    UPROPERTY(meta = (BindWidget))
    class UProgressBar* ManaProgressBar;

    UPROPERTY(meta = (BindWidget))
    class UProgressBar* StaminaProgressBar;

    UPROPERTY(meta = (BindWidget))
    class UImage* Image_HitOverlay;

    FTimerHandle HitFadeTimerHandle;
    void StartHitOverlayFadeOut();
    void FadeOutHitOverlay();

    float PreviousHealth = -1.0f;

    UPROPERTY()
    class UUserWidget* WFCWarningWidgetInstance;

    UPROPERTY()
    class UUserWidget* WFCDoneWidgetInstance;

    UPROPERTY(EditDefaultsOnly, Category = "WFC UI")
    TSubclassOf<UUserWidget> WFCWarningWidgetClass;

    UPROPERTY(EditDefaultsOnly, Category = "WFC UI")
    TSubclassOf<UUserWidget> WFCDoneWidgetClass;

    UPROPERTY(meta = (BindWidget))
    UImage* HotkeyIcon_1;

    UPROPERTY(meta = (BindWidget))
    UImage* HotkeyIcon_2;

    UPROPERTY(meta = (BindWidget))
    UImage* HotkeyIcon_3;

    UPROPERTY(meta = (BindWidget))
    UImage* HotkeyIcon_4;

    UPROPERTY(meta = (BindWidget))
    UImage* HotkeyIcon_5;

    UFUNCTION(BlueprintCallable)
    void UpdateHotkeySlot(int32 Index, const FItemData& ItemData);


    
};

// Fill out your copyright notice in the Description page of Project Settings.


#include "UCharacterHUDWidget.h"
#include "Components/ProgressBar.h"
#include "Components/Image.h"

void UUCharacterHUDWidget::UpdateHealth(float CurrentHealth, float MaxHealth)
{


    if (HealthProgressBar)
    {
        float HealthPercent = CurrentHealth / MaxHealth;
        HealthProgressBar->SetPercent(HealthPercent);
    }

    if (!Image_HitOverlay) return;

    // 실제로 체력이 줄어든 경우에만 피격 효과 발생
    if (PreviousHealth < 0.0f || CurrentHealth < PreviousHealth)
    {
        float NormalizedHealth = CurrentHealth / MaxHealth;
        float OverlayAlpha = FMath::Clamp(1.0f - NormalizedHealth, 0.0f, 0.6f);

        Image_HitOverlay->SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, OverlayAlpha));
        Image_HitOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);

        StartHitOverlayFadeOut(); // 이때만 페이드 시작
    }

    PreviousHealth = CurrentHealth;

    if (WFCWarningWidgetClass)
    {
        WFCWarningWidgetInstance = CreateWidget<UUserWidget>(GetWorld(), WFCWarningWidgetClass);
    }

    if (WFCDoneWidgetClass)
    {
        WFCDoneWidgetInstance = CreateWidget<UUserWidget>(GetWorld(), WFCDoneWidgetClass);
    }


}

void UUCharacterHUDWidget::UpdateMana(float CurrentMana, float MaxMana)
{
    if (ManaProgressBar)
    {
        float ManaPercent = CurrentMana / MaxMana;
        ManaProgressBar->SetPercent(ManaPercent);
    }
}

void UUCharacterHUDWidget::UpdateStamina(float CurrentStamina, float MaxStamina)
{
    if (StaminaProgressBar)
    {
        float StaminaPercent = CurrentStamina / MaxStamina;
        StaminaProgressBar->SetPercent(StaminaPercent);
    }
}

void UUCharacterHUDWidget::StartHitOverlayFadeOut()
{
    // 초기 알파값 유지
    if (Image_HitOverlay)
    {
        Image_HitOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    }

    // 타이머 시작 (0.05초 간격으로 알파 줄이기)
    GetWorld()->GetTimerManager().ClearTimer(HitFadeTimerHandle);
    GetWorld()->GetTimerManager().SetTimer(HitFadeTimerHandle, this, &UUCharacterHUDWidget::FadeOutHitOverlay, 1.0f, true);
}

void UUCharacterHUDWidget::FadeOutHitOverlay()
{
    if (!Image_HitOverlay) return;

    float CurrentAlpha = Image_HitOverlay->ColorAndOpacity.A;
    float NewAlpha = FMath::Clamp(CurrentAlpha - 0.03f, 0.0f, 1.0f);

    Image_HitOverlay->SetColorAndOpacity(FLinearColor(1.0f, 1.0f, 1.0f, NewAlpha));

    if (NewAlpha <= 0.01f)
    {
        // 완료 시 타이머 정지, 안 보이게 처리
        GetWorld()->GetTimerManager().ClearTimer(HitFadeTimerHandle);
        Image_HitOverlay->SetVisibility(ESlateVisibility::Hidden);
    }
}

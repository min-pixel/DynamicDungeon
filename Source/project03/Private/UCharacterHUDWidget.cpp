// Fill out your copyright notice in the Description page of Project Settings.


#include "UCharacterHUDWidget.h"
#include "Components/ProgressBar.h"
#include "SlotWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"

void UUCharacterHUDWidget::UpdateHealth(float CurrentHealth, float MaxHealth)
{

    if (!IsValid(this))
    {
        UE_LOG(LogTemp, Error, TEXT("UpdateHealth: HUD Widget is not valid"));
        return;
    }

    if (HealthProgressBar)
    {
        float HealthPercent = CurrentHealth / MaxHealth;
        HealthProgressBar->SetPercent(HealthPercent);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("HealthProgressBar is NULL in UpdateHealth"));
    }

    if (HealthText)
    {
        FString HealthString = FString::Printf(TEXT("%.0f/%.0f"), CurrentHealth, MaxHealth);
        HealthText->SetText(FText::FromString(HealthString));
    }

   
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

    if (!IsValid(this))
    {
        UE_LOG(LogTemp, Error, TEXT("UpdateHealth: HUD Widget is not valid"));
        return;
    }

    if (ManaProgressBar)
    {
        float ManaPercent = CurrentMana / MaxMana;
        ManaProgressBar->SetPercent(ManaPercent);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("ManaProgressBar is NULL in UpdateMana"));
    }

    if (ManaText)
    {
        FString ManaString = FString::Printf(TEXT("%.0f/%.0f"), CurrentMana, MaxMana);
        ManaText->SetText(FText::FromString(ManaString));
    }

}

void UUCharacterHUDWidget::UpdateStamina(float CurrentStamina, float MaxStamina)
{

    if (!IsValid(this))
    {
        UE_LOG(LogTemp, Error, TEXT("UpdateHealth: HUD Widget is not valid"));
        return;
    }

    if (StaminaProgressBar)
    {
        float StaminaPercent = CurrentStamina / MaxStamina;
        StaminaProgressBar->SetPercent(StaminaPercent);
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("StaminaProgressBar is NULL in UpdateStamina"));
    }

    if (StaminaText)
    {
        FString StaminaString = FString::Printf(TEXT("%.0f/%.0f"), CurrentStamina, MaxStamina);
        StaminaText->SetText(FText::FromString(StaminaString));
    }

}

void UUCharacterHUDWidget::StartHitOverlayFadeOut()
{
    // �ʱ� ���İ� ����
    if (Image_HitOverlay)
    {
        Image_HitOverlay->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
    }

    // Ÿ�̸� ���� (0.05�� �������� ���� ���̱�)
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
        // �Ϸ� �� Ÿ�̸� ����, �� ���̰� ó��
        GetWorld()->GetTimerManager().ClearTimer(HitFadeTimerHandle);
        Image_HitOverlay->SetVisibility(ESlateVisibility::Hidden);
    }
}


void UUCharacterHUDWidget::UpdateHotkeySlot(int32 Index, const FItemData& ItemData)
{
    UImage* TargetIcon = nullptr;

    switch (Index)
    {
    case 0: TargetIcon = HotkeyIcon_1; break;
    case 1: TargetIcon = HotkeyIcon_2; break;
    case 2: TargetIcon = HotkeyIcon_3; break;
    case 3: TargetIcon = HotkeyIcon_4; break;
    case 4: TargetIcon = HotkeyIcon_5; break;
    default: 
        UE_LOG(LogTemp, Warning, TEXT("UpdateHotkeySlot: Invalid Index = %d"), Index);
        return;
    }

    if (!TargetIcon)
    {
        UE_LOG(LogTemp, Error, TEXT("UpdateHotkeySlot: TargetIcon is null for Index = %d"), Index);
        return;
    }

    // �������� �ִ� ���
    if (ItemData.ItemClass && ItemData.ItemIcon)
    {
        TargetIcon->SetBrushFromTexture(ItemData.ItemIcon);
        TargetIcon->SetVisibility(ESlateVisibility::Visible);
        UE_LOG(LogTemp, Log, TEXT("UpdateHotkeySlot: Set icon for slot %d - %s"), Index, *ItemData.ItemName);
    }
    else
    {
        // �� ������ ��� - �����ϰ� ����ų� �⺻ �̹��� ����
        TargetIcon->SetBrushFromTexture(nullptr);
        TargetIcon->SetColorAndOpacity(FLinearColor::White); // ȸ�� ������
        TargetIcon->SetVisibility(ESlateVisibility::Visible);
        UE_LOG(LogTemp, Log, TEXT("UpdateHotkeySlot: Cleared slot %d"), Index);
    }
}


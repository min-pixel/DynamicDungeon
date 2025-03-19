// Fill out your copyright notice in the Description page of Project Settings.


#include "UCharacterHUDWidget.h"
#include "Components/ProgressBar.h"

void UUCharacterHUDWidget::UpdateHealth(float CurrentHealth, float MaxHealth)
{
    if (HealthProgressBar)
    {
        float HealthPercent = CurrentHealth / MaxHealth;
        HealthProgressBar->SetPercent(HealthPercent);
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


// Fill out your copyright notice in the Description page of Project Settings.


#include "WaitForPlayersWidget.h"
#include "Components/TextBlock.h"

void UWaitForPlayersWidget::UpdateStatus(int32 ReadyCount, int32 TotalCount)
{
    if (StatusText)
    {
        FString Status = FString::Printf(TEXT("Another player is getting ready... (%d / %d)"), ReadyCount, TotalCount);
        StatusText->SetText(FText::FromString(Status));
    }
}

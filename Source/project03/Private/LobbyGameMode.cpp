// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyGameMode.h"
#include "LobbyWidget.h"
#include "Kismet/GameplayStatics.h"
#include "MyDCharacter.h"
#include "Blueprint/UserWidget.h"

ALobbyGameMode::ALobbyGameMode()
{
	PlayerControllerClass = APlayerController::StaticClass();

}




void ALobbyGameMode::BeginPlay()
{
	Super::BeginPlay();

	LobbyWidgetClass = StaticLoadClass(UUserWidget::StaticClass(), nullptr, TEXT("/Game/BP/UI/LobbyWidget_BP.LobbyWidget_BP_C"));

	//PlayerControllerClass = APlayerController::StaticClass();


	if (LobbyWidgetClass)
	{
		LobbyWidgetInstance = CreateWidget<ULobbyWidget>(GetWorld(), LobbyWidgetClass);
		if (LobbyWidgetInstance)
		{
			LobbyWidgetInstance->AddToViewport(1);

			// 약간의 딜레이를 줘서 Pawn이 생성되기를 기다림
			
			GetWorld()->GetTimerManager().SetTimer(DelayHandle, [this]()
				{
					APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
					if (PC)
					{
						AMyDCharacter* PlayerCharacter = Cast<AMyDCharacter>(PC->GetPawn());
						if (LobbyWidgetInstance)
						{
							LobbyWidgetInstance->InitializeLobby(PlayerCharacter);
						}

						PC->bShowMouseCursor = true;
						FInputModeUIOnly InputMode;
						InputMode.SetWidgetToFocus(LobbyWidgetInstance->TakeWidget());
						InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
						PC->SetInputMode(InputMode);
					}
					else
					{
						LobbyWidgetInstance->InitializeLobby(nullptr);
						PC->bShowMouseCursor = true;
						FInputModeUIOnly InputMode;
						InputMode.SetWidgetToFocus(LobbyWidgetInstance->TakeWidget());
						InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
						PC->SetInputMode(InputMode);
					}

				}, 0.1f, false);
			

			
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("LobbyWidgetClass not assigned in LobbyGameMode!"));
	}

}

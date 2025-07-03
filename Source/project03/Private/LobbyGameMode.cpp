// Fill out your copyright notice in the Description page of Project Settings.


#include "LobbyGameMode.h"
#include "LobbyWidget.h"
#include "Kismet/GameplayStatics.h"
#include "MyDCharacter.h"
#include "MyPlayerController.h"
#include "Blueprint/UserWidget.h"

ALobbyGameMode::ALobbyGameMode()
{
	//PlayerControllerClass = APlayerController::StaticClass();

	PlayerControllerClass = AMyPlayerController::StaticClass();

}




void ALobbyGameMode::BeginPlay()
{
	Super::BeginPlay();

	//if (!HasAuthority())
	//{
	//	return;  // Ŭ�󿡼��� UI ���� X
	//}

	LobbyWidgetClass = StaticLoadClass(UUserWidget::StaticClass(), nullptr, TEXT("/Game/BP/UI/LobbyWidget_BP.LobbyWidget_BP_C"));

	//PlayerControllerClass = APlayerController::StaticClass();


	//if (LobbyWidgetClass)
	//{
	//	LobbyWidgetInstance = CreateWidget<ULobbyWidget>(GetWorld(), LobbyWidgetClass);
	//	if (LobbyWidgetInstance)
	//	{
	//		LobbyWidgetInstance->AddToViewport(1);

	//		// �ణ�� �����̸� �༭ Pawn�� �����Ǳ⸦ ��ٸ�
	//		
	//		GetWorld()->GetTimerManager().SetTimer(DelayHandle, [this]()
	//			{
	//				APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	//				for (FConstPlayerControllerIterator Iterator = GetWorld()->GetPlayerControllerIterator(); Iterator; ++Iterator)
	//				{
	//					APlayerController* EachPC = Iterator->Get();
	//					if (EachPC && EachPC->IsLocalController())
	//					{
	//						AMyDCharacter* PlayerCharacter = Cast<AMyDCharacter>(EachPC->GetPawn());
	//						LobbyWidgetInstance->InitializeLobby(PlayerCharacter);
	//						EachPC->bShowMouseCursor = true;

	//						FInputModeUIOnly InputMode;
	//						InputMode.SetWidgetToFocus(LobbyWidgetInstance->TakeWidget());
	//						InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	//						EachPC->SetInputMode(InputMode);
	//					}
	//				}
	//				/*if (PC)
	//				{
	//					AMyDCharacter* PlayerCharacter = Cast<AMyDCharacter>(PC->GetPawn());
	//					if (LobbyWidgetInstance)
	//					{
	//						LobbyWidgetInstance->InitializeLobby(PlayerCharacter);
	//					}

	//					PC->bShowMouseCursor = true;
	//					FInputModeUIOnly InputMode;
	//					InputMode.SetWidgetToFocus(LobbyWidgetInstance->TakeWidget());
	//					InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	//					PC->SetInputMode(InputMode);
	//				}
	//				else
	//				{
	//					LobbyWidgetInstance->InitializeLobby(nullptr);
	//					PC->bShowMouseCursor = true;
	//					FInputModeUIOnly InputMode;
	//					InputMode.SetWidgetToFocus(LobbyWidgetInstance->TakeWidget());
	//					InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	//					PC->SetInputMode(InputMode);
	//				}*/

	//			}, 0.1f, false);
	//		

	//		
	//	}
	//}
	//else
	//{
	//	UE_LOG(LogTemp, Error, TEXT("LobbyWidgetClass not assigned in LobbyGameMode!"));
	//}

}

void ALobbyGameMode::CheckAllPlayersReady()
{
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AMyPlayerController* PC = Cast<AMyPlayerController>(It->Get());
		if (PC && !PC->bIsReady)
		{
			return; // ���� �غ� �� �� ��� ����
		}
	}

	// ��� �غ� �Ϸ� �� ���� Ʈ����
	UGameplayStatics::OpenLevel(this, TEXT("GameMap"), true, TEXT("listen"));
}
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
	bUseSeamlessTravel = true;
}




void ALobbyGameMode::BeginPlay()
{
	Super::BeginPlay();

	//if (!HasAuthority())
	//{
	//	return;  // 클라에서는 UI 생성 X
	//}

	LobbyWidgetClass = StaticLoadClass(UUserWidget::StaticClass(), nullptr, TEXT("/Game/BP/UI/LobbyWidget_BP.LobbyWidget_BP_C"));

	//PlayerControllerClass = APlayerController::StaticClass();


	//if (LobbyWidgetClass)
	//{
	//	LobbyWidgetInstance = CreateWidget<ULobbyWidget>(GetWorld(), LobbyWidgetClass);
	//	if (LobbyWidgetInstance)
	//	{
	//		LobbyWidgetInstance->AddToViewport(1);

	//		// 약간의 딜레이를 줘서 Pawn이 생성되기를 기다림
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
	bool bAllReady = true;

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AMyPlayerController* PC = Cast<AMyPlayerController>(It->Get());
		if (PC && !PC->bIsReady)
		{
			bAllReady = false;
			break;
		}
	}

	if (bAllReady)
	{
		// 마지막 준비 상태 (5/5) 먼저 브로드캐스트
		BroadcastReadyStatus();

		// 약간의 딜레이 후 이동
		FTimerHandle TravelTimerHandle;
		GetWorld()->GetTimerManager().SetTimer(TravelTimerHandle, this, &ALobbyGameMode::StartTravel, 1.0f, false);
	}
	else
	{
		BroadcastReadyStatus();
	}
}

void ALobbyGameMode::StartTravel()
{
	GetWorld()->ServerTravel("/Game/DynamicDugeon?listen");
}


int32 ALobbyGameMode::GetReadyCount() const
{
	int32 Count = 0;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AMyPlayerController* PC = Cast<AMyPlayerController>(It->Get());
		if (PC && PC->bIsReady)
		{
			Count++;
		}
	}
	return Count;
}

int32 ALobbyGameMode::GetNotReadyCount() const
{
	int32 Total = 0;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		Total++;
	}
	return Total - GetReadyCount();
}

//void ALobbyGameMode::BroadcastReadyStatus()
//{
//	int32 ReadyCount = 0;
//	int32 NotReadyCount = 0;
//
//	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
//	{
//		AMyPlayerController* PC = Cast<AMyPlayerController>(It->Get());
//		if (PC)
//		{
//			if (PC->bIsReady) ReadyCount++;
//			else NotReadyCount++;
//		}
//	}
//
//	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
//	{
//		AMyPlayerController* PC = Cast<AMyPlayerController>(It->Get());
//		if (PC)
//		{
//			PC->ClientUpdateWaitWidget(ReadyCount, NotReadyCount);
//		}
//	}
//}

void ALobbyGameMode::BroadcastReadyStatus()
{
	int32 Total = 0;
	int32 ReadyCount = 0;

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AMyPlayerController* PC = Cast<AMyPlayerController>(It->Get());
		if (PC)
		{
			Total++;
			if (PC->bIsReady)
			{
				ReadyCount++;
			}
		}
	}

	// 모든 클라에게 현재 준비 상태 전송
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		AMyPlayerController* PC = Cast<AMyPlayerController>(It->Get());
		if (PC)
		{
			PC->ClientUpdateWaitWidget(ReadyCount, Total);
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[Broadcast] ReadyCount: %d, Total: %d"), ReadyCount, Total);
}